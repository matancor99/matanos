#include "keyboard.h"
#include "ports.h"
#include "../cpu/isr.h"
#include "screen.h"
#include "../kernel/util.h"
#include "../kernel/kernel.h"



#define KEYBOARD_DATA_REGISTER 0x60
#define KEYBOARD_CONTROL_REGISTER 0x64
#define TEST_COMMAND 0xAA


#define REG_DATA 0x60
#define REG_CMD 0x64
#define REG_STATUS 0x64

/* PS/2 controller commands */
#define CMD_CONFIG_READ 0x20
#define CMD_CONFIG_WRITE 0x60
/* if the second channel doesn't exist, these will simply be ignored */
#define CMD_PORT2_DISABLE 0xa7
#define CMD_PORT2_ENABLE 0xa8
#define CMD_PORT2_TEST 0xa9
#define CMD_SELF_TEST 0xaa
#define CMD_PORT1_TEST 0xab
#define CMD_PORT1_ENABLE 0xae
#define CMD_PORT1_DISABLE 0xad
#define CMD_WRITE_TO_PORT2 0xd4

#define DEVICE_CMD_RESET 0xff
#define DEVICE_RESPONSE_RESET_ACK 0xfa
#define DEVICE_RESPONSE_RESET_NACK 0xfc

#define PORT_SEND_NUM_RETRIES 1000

enum ps2_port {
    PORT1,
    PORT2
};

/* self test responses */
#define SELF_TEST_SUCCESS 0x55
#define SELF_TEST_FAILED 0xfc

enum {
    PS2_BUF_STATUS_CLEAR,
    PS2_BUF_STATUS_FULL
};

enum {
    DEVICE_DATA,
    CONTROLLER_CMD
};

enum {
    STATUS_REG_NOERROR,
    STATUS_REG_ERROR
};

enum {
    IRQ_STATUS_DISABLED,
    IRQ_STATUS_ENABLED
};

enum {
    SYSTEM_FLAG_POST_ERROR,
    SYSTEM_FLAG_POST_SUCCESS
};

enum {
    CLOCK_STATUS_ENABLED,
    CLOCK_STATUS_DISABLED
};

enum {
    TRANSLATION_DISABLED,
    TRANSLATION_ENABLED
};

struct ps2_status {
    uint8_t tx_buf_status : 1; /* must be set before attempting to read data reg */
    uint8_t rx_buf_status : 1; /* must be unset before attempting to write to data reg */
    uint8_t system_flag : 1;
    uint8_t cmd_or_data : 1;
    uint8_t unknown1 : 1;
    uint8_t unknown2 : 1;
    uint8_t timeout : 1;
    uint8_t parity : 1;
};
static_assert_sizeof(struct ps2_status, sizeof(uint8_t));

struct ps2_config {
    uint8_t port1_irq_status : 1;
    uint8_t port2_irq_status : 1;
    uint8_t system_flag : 1;
    uint8_t zero1 : 1;
    uint8_t port1_clock_status : 1;
    uint8_t port2_clock_status : 1;
    uint8_t port1_translation_status : 1;
    uint8_t zero2 : 1;
};
static_assert_sizeof(struct ps2_config, sizeof(uint8_t));

static bool is_dual_channel = false;
static bool should_probe_second_channel = true;

#define BACKSPACE 0x0E
#define ENTER 0x1C
#define LEFT_SHIFT 0x2A
#define CAPS_LOCK 0x3A
static char key_buffer[256];
#define SC_MAX (58)
#define REL_KEY_DIFF (0x80)

static bool is_shift_pressed = false;
static bool is_caps_locked = false;



const char *sc_name[] = { "ERROR", "Esc", "1", "2", "3", "4", "5", "6",
                          "7", "8", "9", "0", "-", "=", "Backspace", "Tab", "Q", "W", "E",
                          "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Enter", "Lctrl",
                          "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "'", "`",
                          "LShift", "\\", "Z", "X", "C", "V", "B", "N", "M", ",", ".",
                          "/", "RShift", "Keypad *", "LAlt", "Spacebar", "CAPSLOCK"};
const char sc_ascii_shift[] = { '?', '?', '1', '2', '3', '4', '5', '6',
                          '7', '8', '9', '0', '-', '=', '?', '?', 'Q', 'W', 'E', 'R', 'T', 'Y',
                          'U', 'I', 'O', 'P', '[', ']', '?', '?', 'A', 'S', 'D', 'F', 'G',
                          'H', 'J', 'K', 'L', ':', '\'', '`', '?', '\\', 'Z', 'X', 'C', 'V',
                          'B', 'N', 'M', '<', '>', '?', '?', '?', '?', ' ', '?', '?'};

const char sc_ascii[] = { '?', '?', '!', '@', '#', '$', '%', '^',
                          '&', '*', '(', ')', '_', '+', '?', '?', 'q', 'w', 'e', 'r', 't', 'y',
                          'u', 'i', 'o', 'p', '{', '}', '?', '?', 'a', 's', 'd', 'f', 'g',
                          'h', 'j', 'k', 'l', ';', '\'', '`', '?', '\\', 'z', 'x', 'c', 'v',
                          'b', 'n', 'm', ',', '.', '/', '?', '?', '?', ' ' , '?', '?'};

static void keyboard_callback(registers_t regs) {
    /* The PIC leaves us the scancode in port 0x60 */
    unsigned char scancode = port_byte_in(KEYBOARD_DATA_REGISTER);
    bool is_rel_key = false;
    bool is_press_key = false;
    const char * ascii_table = sc_ascii;

    //Check if the key is pressed or released

    if (scancode > REL_KEY_DIFF +  SC_MAX) {
        is_rel_key = true;
        goto cleanup;
    }
    else if (scancode >= REL_KEY_DIFF) {
        scancode -= REL_KEY_DIFF;
        is_rel_key = true;
    }
    else if (scancode > SC_MAX) {
        goto cleanup;
    }
    else {
        is_press_key = true;
    }

    //Shift handling
    if (scancode == LEFT_SHIFT) {
        if(is_rel_key) {
            is_shift_pressed = false;
        }
        if(is_press_key) {
            is_shift_pressed = true;
        }
        goto cleanup;
    }


    // Printing handling + Spacial keys - BACKSPACE, ENTER, CAPSLOCK
    if (is_rel_key) {
        goto cleanup;
    }
    else if (scancode == CAPS_LOCK) {
        is_caps_locked = !is_caps_locked;
    }
    else if (scancode == BACKSPACE) {
        backspace(key_buffer);
        kprint_backspace();
    } else if (scancode == ENTER) {
        kprint("\n");
        user_input(key_buffer); /* kernel-controlled function */
        key_buffer[0] = '\0';
    } else {
        if(is_caps_locked ^ is_shift_pressed) {
            ascii_table = sc_ascii_shift;
        }
        else {
            ascii_table = sc_ascii;
        }
        char letter = ascii_table[(int)scancode];
        /* Remember that kprint only accepts char[] */
        char str[2] = {letter, '\0'};
        append(key_buffer, letter);
        kprint(str);
    }
    UNUSED(regs);
cleanup:
    return;
}

static void wait_keyboard(void) {
    unsigned char c = port_byte_in(KEYBOARD_CONTROL_REGISTER);
    while (c & 0x2) {
        kprint("Keyboard not ready\n");
        char ac[4];
        int_to_ascii(c, ac);
        kprint("control status: ");
        kprint(ac);
    }
}


void init_A20(void) {

    wait_keyboard();
    port_byte_out(KEYBOARD_CONTROL_REGISTER, 0xAD);
    wait_keyboard();
    port_byte_out(KEYBOARD_CONTROL_REGISTER, 0xD0);
    io_wait();


    unsigned char d = port_byte_in(KEYBOARD_DATA_REGISTER);
    char ad[8];
    hex_to_ascii(d, ad);
    kprint("output port status before: ");
    kprint(ad);
    kprint("\n");

    wait_keyboard();
    port_byte_out(KEYBOARD_CONTROL_REGISTER, 0xD1);
    wait_keyboard();
    port_byte_out(KEYBOARD_DATA_REGISTER, d | 2);

    wait_keyboard();
    port_byte_out(KEYBOARD_CONTROL_REGISTER, 0xAE);

}

static void read_status(struct ps2_status *reg)
{
    *(uint8_t *)reg = port_byte_in(REG_STATUS);
}

static void read_config(struct ps2_config *config)
{
    port_byte_out(REG_CMD, CMD_CONFIG_READ);
    *(uint8_t *)config = port_byte_in(REG_DATA);
}

static void write_config(const struct ps2_config *config)
{
    port_byte_out(REG_CMD, CMD_CONFIG_WRITE);
    port_byte_out(REG_DATA, *(uint8_t *)config);
}

static bool i8042_self_test(void)
{
    uint8_t response;
    port_byte_out(REG_CMD, CMD_SELF_TEST);
    response = port_byte_in(REG_DATA);

    while (response != SELF_TEST_SUCCESS && response != SELF_TEST_FAILED)
        response = port_byte_in(REG_DATA);

    return response == SELF_TEST_SUCCESS;
}

void reboot(void) {
    wait_keyboard();
    port_byte_out(KEYBOARD_CONTROL_REGISTER, 0xFE);
}

static void disable_devices(void)
{
    struct ps2_status status_reg;
    struct ps2_config config;
    port_byte_out(REG_CMD, CMD_PORT1_DISABLE);
    port_byte_out(REG_CMD, CMD_PORT2_DISABLE);
    read_status(&status_reg);

    /* flush output buffer */
    while (status_reg.tx_buf_status != PS2_BUF_STATUS_CLEAR) {
        io_wait();
        (void)port_byte_in(REG_DATA);
        read_status(&status_reg);
    }

    read_config(&config);
    config.port1_irq_status = IRQ_STATUS_DISABLED;
    config.port2_irq_status = IRQ_STATUS_DISABLED;
    config.port1_translation_status = TRANSLATION_DISABLED;

    if (config.port2_clock_status == CLOCK_STATUS_ENABLED) {
        /*
         * we detected the clock is enabled even though we've disabled the 2nd port. this weird behavior means that
         * there's definitely no second channel.
         */
        is_dual_channel = false;
        should_probe_second_channel = false;
        printf("PS/2 2nd channel disabled.\n");
    }

    write_config(&config);
}

static bool is_channel2_present(void)
{
    struct ps2_config config;
    port_byte_out(REG_CMD, CMD_PORT2_ENABLE);
    read_config(&config);
    bool ret = config.port2_clock_status == CLOCK_STATUS_ENABLED;
    port_byte_out(REG_CMD, CMD_PORT2_DISABLE);
    return ret;
}

static bool test_ports(void)
{
    port_byte_out(REG_CMD, CMD_PORT1_TEST);
    uint8_t response = port_byte_in(REG_DATA);

    if (response != 0)
        return false;

    if (is_dual_channel) {
        port_byte_out(REG_CMD, CMD_PORT2_TEST);
        response = port_byte_in(REG_DATA);

        if (response != 0)
            return false;
    }

    return true;
}

static void enable_ports(void)
{
    struct ps2_config config = {0};
    port_byte_out(REG_CMD, CMD_PORT1_ENABLE);
    port_byte_out(REG_CMD, CMD_PORT2_ENABLE);
    read_config(&config);
    config.port1_irq_status = IRQ_STATUS_ENABLED;
    config.port2_irq_status = IRQ_STATUS_ENABLED;
    write_config(&config);
}

static bool send_port(enum ps2_port port, uint8_t data)
{
    struct ps2_status status;
    read_status(&status);
    int retries = PORT_SEND_NUM_RETRIES;

    if (port == PORT2) {
        if (!is_dual_channel)
            return false;

        port_byte_out(REG_CMD, CMD_WRITE_TO_PORT2);
    }

    while (status.rx_buf_status != PS2_BUF_STATUS_CLEAR && retries > 0) {
        io_wait();
        read_status(&status);
        --retries;
    }

    if (retries == 0)
        return false;

    port_byte_out(REG_DATA, data);
    return true;
}

static bool reset_devices(void)
{
    bool ret = send_port(PORT1, DEVICE_CMD_RESET);

    if (!ret)
        return ret;

    if (is_dual_channel)
        ret = send_port(PORT2, DEVICE_CMD_RESET);

    return ret;
}

static bool set_keyboard_scancode(void) {
    bool ret = send_port(PORT1, 0xF0);
    if (!ret)
        return ret;
    ret = send_port(PORT1, 0x01);
    return ret;
}

// Init sequence as described in osdev
bool init_keyboard() {
    disable_devices();

    if (!i8042_self_test()) {
        printf("PS/2 controller self test failed!!!\n");
        return false;

    }

    if (should_probe_second_channel) {
        /* make sure the second channel is present. */
        is_dual_channel = is_channel2_present();
    }

    if (!test_ports()) {
        printf("PS2 ports test failed.\n");
        return false;
    }

    enable_ports();

    register_interrupt_handler(IRQ1, keyboard_callback);

    if (!reset_devices()) {
        printf("Failed to send reset command to PS2 devices\n");
        return false;
    }

    if (!set_keyboard_scancode()) {
        printf("Failed to set the scancode\n");
    }

    printf("\n> ");
    return true;
}



