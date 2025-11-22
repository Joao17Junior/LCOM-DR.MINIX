#include "keyboard.h"
#include "KBC.h"

uint8_t scancode = 0;
int keyboard_hook_id = 1;
static int is_subscribed = 0;

int (keyboard_subscribe_interrupts)(uint8_t *bit_no) {
    if (is_subscribed) return 1; // Already subscribed!
    if (bit_no == NULL) return 1;
    *bit_no = keyboard_hook_id;
    int r = sys_irqsetpolicy(IRQ_KEYBOARD, IRQ_REENABLE | IRQ_EXCLUSIVE, &keyboard_hook_id);
    if (r == 0) is_subscribed = 1;
    return r;
}

int (keyboard_unsubscribe_interrupts)() {
    is_subscribed = 0;
    int irqrm = sys_irqrmpolicy(&keyboard_hook_id);
    keyboard_hook_id = 1; // Reset hook ID
    return irqrm;
}

void (kbc_ih)() {
    if (read_KBC_output(KBC_OUT_CMD, &scancode, 0) != 0) printf("Error: Could not read scancode!\n");
}

int (keyboard_restore)() {
    uint8_t commandByte;

    if (write_KBC_command(KBC_IN_CMD, KBC_READ_CMD) != 0) return 1;          
    if (read_KBC_output(KBC_OUT_CMD, &commandByte, 0) != 0) return 1; 

    commandByte |= ENABLE_INT;  

    if (write_KBC_command(KBC_IN_CMD, KBC_WRITE_CMD) != 0) return 1;    
    if (write_KBC_command(KBC_WRITE_CMD, commandByte) != 0) return 1;

    return 0;
}
