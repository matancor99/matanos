#include "keyboard.h"
#include "ports.h"
#include "../cpu/isr.h"
#include "screen.h"
#include "../kernel/util.h"
#include "../kernel/kernel.h"

#include <stdbool.h>

#define KEYBOARD_DATA_REGISTER 0x60
#define KEYBOARD_CONTROL_REGISTER 0x64
#define TEST_COMMAND 0xAA

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

void init_keyboard() {
    wait_keyboard();
    port_byte_out(KEYBOARD_CONTROL_REGISTER, TEST_COMMAND);
    io_wait();
    unsigned char d = port_byte_in(KEYBOARD_DATA_REGISTER);
    if (d == 0x55) {
        kprint("Test Passed\n");
    }

    register_interrupt_handler(IRQ1, keyboard_callback);
    kprint("\n> ");
}



