#include "keyboard.h"
#include "ports.h"
#include "../cpu/isr.h"
#include "screen.h"
#include "../kernel/util.h"
#include "../kernel/kernel.h"

#define KEYBOARD_DATA_REGISTER 0x60
#define KEYBOARD_CONTROL_REGISTER 0x64
#define TEST_COMMAND 0xAA

#define BACKSPACE 0x0E
#define ENTER 0x1C
static char key_buffer[256];
#define SC_MAX 57
const char *sc_name[] = { "ERROR", "Esc", "1", "2", "3", "4", "5", "6",
                          "7", "8", "9", "0", "-", "=", "Backspace", "Tab", "Q", "W", "E",
                          "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Enter", "Lctrl",
                          "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "'", "`",
                          "LShift", "\\", "Z", "X", "C", "V", "B", "N", "M", ",", ".",
                          "/", "RShift", "Keypad *", "LAlt", "Spacebar"};
const char sc_ascii[] = { '?', '?', '1', '2', '3', '4', '5', '6',
                          '7', '8', '9', '0', '-', '=', '?', '?', 'Q', 'W', 'E', 'R', 'T', 'Y',
                          'U', 'I', 'O', 'P', '[', ']', '?', '?', 'A', 'S', 'D', 'F', 'G',
                          'H', 'J', 'K', 'L', ';', '\'', '`', '?', '\\', 'Z', 'X', 'C', 'V',
                          'B', 'N', 'M', ',', '.', '/', '?', '?', '?', ' '};


static void keyboard_callback(registers_t regs) {
    /* The PIC leaves us the scancode in port 0x60 */
    unsigned char scancode = port_byte_in(KEYBOARD_DATA_REGISTER);

    if (scancode > SC_MAX) return;
    if (scancode == BACKSPACE) {
        backspace(key_buffer);
        kprint_backspace();
    } else if (scancode == ENTER) {
        kprint("\n");
        user_input(key_buffer); /* kernel-controlled function */
        key_buffer[0] = '\0';
    } else {
        char letter = sc_ascii[(int)scancode];
        /* Remember that kprint only accepts char[] */
        char str[2] = {letter, '\0'};
        append(key_buffer, letter);
        kprint(str);
    }
    UNUSED(regs);
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

void init_keyboard() {
    wait_keyboard();
    port_byte_out(KEYBOARD_CONTROL_REGISTER, TEST_COMMAND);
    io_wait();
    unsigned char d = port_byte_in(KEYBOARD_DATA_REGISTER);
    if (d == 0x55) {
        kprint("Test Passed\n");
    }

    register_interrupt_handler(IRQ1, keyboard_callback);

}



