#include <lcom/lcf.h>
#include <lcom/lab3.h>
#include <stdbool.h>
#include <stdint.h>
#include <minix/sysutil.h>
#include <minix/syslib.h>
#include <minix/drivers.h>
#include "handlers.c"

#define KBD_IRQ 1
#define KBD_OUT_BUF 0x60
#define KBD_STAT_REG 0x64
#define KBD_OBF 0x01
#define KBC_ST_REG 0x64
#define KBC_OBF 0x01
#define KBD_PAR_ERR 0x80
#define KBD_TO_ERR 0x40
#define WAIT_KBD 20000 // Define a suitable delay value in microseconds
#define KBD_INT 0x01 // Define the KBD_INT constant
#define KBC_CMD_REG 0x64 // Define the KBC_CMD_REG constant
#define KBC_WRITE_CMD 0x60 // Define the KBC_WRITE_CMD constant

int main(int argc, char *argv[]) {
  // sets the language of LCF messages (can be either EN-US or PT-PT)
  lcf_set_language("EN-US");

  // enables to log function invocations that are being "wrapped" by LCF
  // [comment this out if you don't want/need it]
  lcf_trace_calls("/home/lcom/labs/lab3/trace.txt");

  // enables to save the output of printf function calls on a file
  // [comment this out if you don't want/need it]
  lcf_log_output("/home/lcom/labs/lab3/output.txt");

  // handles control over to LCF
  // [LCF handles command line arguments and invokes the right function]
  if (lcf_start(argc, argv))
    return 1;

  // LCF clean up tasks
  // [must be the last statement before return]
  lcf_cleanup();

  return 0;
}

uint8_t scancode;

int (kbd_test_scan)() { //done
  uint8_t bit_no;
  int ipc_status;
  message msg;
  int r;

  if (kbd_subscribe_int(&bit_no) != 0) {
    return 1;
  }

  int irq_set = BIT(bit_no);

  while (scancode != 0x81) { // 0x81 is the break code for ESC key
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed with: %d", r);
      continue;
    }
    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & irq_set) {
            kbc_ih();
            uint8_t bytes[2];
            uint8_t size = 1;
            bool make = !(scancode & 0x80);
            if (is_two_byte) {
              bytes[0] = 0xE0;
              bytes[1] = scancode;
              size = 2;
              is_two_byte = false;
            } else {
              bytes[0] = scancode;
            }
            kbd_print_scancode(make, size, bytes);
          }
          break;
        default:
          break;
      }
    }
  }

  if (kbd_unsubscribe_int() != 0) {
    return 1;
  }

  kbd_print_no_sysinb(sys_inb_count);

  return 0;
}

int (kbd_test_poll)() { //done
  uint8_t bytes[2];
  uint8_t size = 1;
  bool make;
  bool is_two_byte = false;

  uint8_t command_byte;
  scancode = 0;

  // Polling loop
  while (scancode != 0x81) {
    uint8_t temp_scancode;

    // Read scancode from the KBC
    if (kbc_read_data(&temp_scancode) != 0) {
      continue; // Skip on error
    }

    // Update the global scancode variable
    scancode = temp_scancode;

    // Determine if it's a make or break code
    make = !(scancode & 0x80);

    // Handle two-byte scancodes
    if (scancode == 0xE0) {
      is_two_byte = true;
    } else {
      if (is_two_byte) {
        bytes[0] = 0xE0;
        bytes[1] = scancode;
        size = 2;
        is_two_byte = false;
      } else {
        bytes[0] = scancode;
        size = 1;
      }

      // Print the scancode
      kbd_print_scancode(make, size, bytes);
    }
  }

  // Print the number of sys_inb calls
  kbd_print_no_sysinb(sys_inb_count);

  // Read the current command byte
  if (kbc_read_command_byte(&command_byte) != 0) {
    printf("Failed to read command byte\n");
    return 1;
  }

  // Re-enable keyboard interrupts
  command_byte |= KBD_INT;
  if (kbc_write_command_byte(command_byte) != 0) {
    printf("Failed to write command byte\n");
    return 1;
  }

  return 0;
}

int (kbd_test_timed_scan)(uint8_t idle) {
  uint8_t kbd_bit_no, timer_bit_no;
  int ipc_status, r;
  message msg;

  if (kbd_subscribe_int(&kbd_bit_no) != 0) {
    return 1;
  }
  if (timer_subscribe_int(&timer_bit_no) != 0) {
    kbd_unsubscribe_int(); return 1;
  }

  int kbd_irq_set = BIT(kbd_bit_no);
  int timer_irq_set = BIT(timer_bit_no);
  int lastticdone = 0;

  while (scancode != 0x81 && timer_counter - lastticdone < idle*60) { // 0x81 is the break code for ESC key
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed with: %d", r);
      continue;
    }
    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & kbd_irq_set) {
            kbc_ih();

            uint8_t bytes[2];
            uint8_t size = 1;
            bool make = !(scancode & 0x80);
            if (is_two_byte) {
              bytes[0] = 0xE0;
              bytes[1] = scancode;
              size = 2;
              is_two_byte = false;
            } else {
              bytes[0] = scancode;
            }
            kbd_print_scancode(make, size, bytes);
            lastticdone = timer_counter;
          }
          if (msg.m_notify.interrupts & timer_irq_set) {
            timer_int_handler(); // Increments global counter
          }
          break;
        default:
          break;
      }
    }
  }

  if (kbd_unsubscribe_int() != 0) {
    return 1;
  }

  if (timer_unsubscribe_int() != 0) {
    return 1;
  }

  kbd_print_no_sysinb(sys_inb_count);

  return 0;
}
