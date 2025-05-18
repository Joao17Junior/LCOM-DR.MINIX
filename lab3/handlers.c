#include <lcom/lcf.h>
#include <lcom/lab3.h>
#include <stdbool.h>
#include <stdint.h>
#include <minix/sysutil.h>
#include <minix/syslib.h>
#include <minix/drivers.h>

#define KBD_IRQ 1
#define KBD_OUT_BUF 0x60
#define KBD_STAT_REG 0x64
#define KBD_OBF 0x01
#define KBC_ST_REG 0x64
#define KBC_OBF 0x01
#define KBD_PAR_ERR 0x80
#define KBD_TO_ERR 0x40
#define WAIT_KBD 20000

int hook_id = KBD_IRQ;
uint8_t scancode;
bool is_two_byte = false;
uint32_t sys_inb_count = 0;

int (kbd_subscribe_int)(uint8_t *bit_no) {
  *bit_no = hook_id;
  if (sys_irqsetpolicy(KBD_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id) != OK) {
    return 1;
  }
  return 0;
}

int (kbd_unsubscribe_int)() {
  if (sys_irqrmpolicy(&hook_id) != OK) {
    return 1;
  }
  return 0;
}

int (util_sys_inb)(int port, uint8_t *value) {
  uint32_t val;
  sys_inb_count++;
  if (sys_inb(port, &val) != OK) {
    return 1;
  }
  *value = (uint8_t)val;
  return 0;
}

int (readbyte)(uint8_t *data) {
  uint8_t stat;
  
  while (1) {
      if (util_sys_inb(KBC_ST_REG, &stat) != OK) return -1;

      /* Check if output buffer is full */
      if (stat & KBC_OBF) {
          if (util_sys_inb(KBD_OUT_BUF, data) != OK) return -1;

          /* Check for parity and timeout errors */
          if ((stat & (KBD_PAR_ERR | KBD_TO_ERR)) == 0) {
              return 0; // Success
          } else {
              return -1; // Error reading
          }
      }
      tickdelay(micros_to_ticks(WAIT_KBD)); // Small delay
  }
}

void (kbc_ih)() {
  uint8_t data;

  if (readbyte(&data) == 0) { // Successfully read byte
      if (data == 0xE0) {
          is_two_byte = true;
      } else {
          scancode = data;
      }
  }
}

