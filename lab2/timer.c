#include <lcom/lcf.h>
#include <lcom/timer.h>

#include <stdint.h>

#include "i8254.h"

int hook_id = 0;
uint32_t count = 0;

int (timer_set_frequency)(uint8_t timer, uint32_t freq) {
  uint8_t timer_sel;
  uint8_t timer_reg;
  uint8_t st = 0;

  if(timer == 0){ 
    timer_sel = TIMER_SEL0;
    timer_reg = TIMER_0;
  } 
  else if(timer == 1){ 
    timer_sel = TIMER_SEL1;
    timer_reg = TIMER_1;
  } 
  else if(timer == 2){ 
    timer_sel = TIMER_SEL2;
    timer_reg = TIMER_2;
  } 
  else return 1;
  
  if(timer_get_conf(timer, &st)) return 1;

  uint8_t last4 = st & 0x0E;

  uint8_t cmd = timer_sel | TIMER_LSB_MSB | last4;
  if(sys_outb(TIMER_CTRL,(uint32_t) cmd)) return 1;

  uint16_t clock = TIMER_FREQ / freq;

  uint8_t lsb = 0, msb =0;
  util_get_LSB(clock, &lsb);
  util_get_MSB(clock, &msb);

  if(sys_outb(timer_reg, lsb)) return 1;
  if(sys_outb(timer_reg, msb)) return 1;

  return 0;
}

int (timer_subscribe_int)(uint8_t *bit_no) {
  if(bit_no == NULL) return 1;
  *bit_no = BIT(hook_id);
  if(sys_irqsetpolicy(TIMER0_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id) != 0) return 1;
  return 0;
}

int (timer_unsubscribe_int)() {
  if(sys_irqrmpolicy(&hook_id) != 0) return 1;
  return 0;
}

void (timer_int_handler)() {
   count++;
}

int (timer_get_conf)(uint8_t timer, uint8_t *st) {
  uint8_t TIMER_T = 0;
  int check, check2;

  if(timer == 0){
    TIMER_T = TIMER_0;
}
  else if(timer == 1){
    TIMER_T = TIMER_1;
  }
  else if(timer == 2){
     TIMER_T = TIMER_2;
  }

  uint32_t command = TIMER_RB_CMD | TIMER_RB_COUNT_ | TIMER_RB_SEL(timer);

  check = sys_inb(TIMER_CTRL, &command);
  check2 = util_sys_inb(TIMER_T, st);  //Ler status do timer

  if(check == 0 && check2 == 0)
    return 0;
  return 1;
}

int (timer_display_conf)(uint8_t timer, uint8_t st,
                        enum timer_status_field field) {
union timer_status_field_val conf; 
uint8_t in_mode;

  if(field == tsf_all){
    conf.byte = st;
    }     
  else if(field == tsf_initial){
     in_mode = (st & 0x30) >> 4;

     if(in_mode == 0){
     conf.in_mode = INVAL_val;
    }
     else if(in_mode == 1){
     conf.in_mode = LSB_only;
    }
     else if(in_mode == 2){
     conf.in_mode = MSB_only;
    }
     else if(in_mode == 3){
     conf.in_mode = MSB_after_LSB;
    }
 }  
 else if(field == tsf_mode){
     conf.count_mode = (st & 0x0E) >> 1;
     if(conf.count_mode == 0x6 || conf.count_mode == 0x7)
     conf.count_mode &= 0x03;
    }
  else if(field == tsf_base){ 
    conf.bcd = st & TIMER_BCD;
  }  
  int check;
  check = timer_print_config(timer, field, conf);
  if(check==0)
  return 0;
return 1;

}
