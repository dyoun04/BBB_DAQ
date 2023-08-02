/*
 * This version runs an oscillator on pin XXX to drive the A/D.
 * The period of the oscillator is set by the ARM which sets the
 * oscillator period on start-up.
 *
 */
#include <stdint.h>
#include "pru_cfg.h"
#include "pru_intc.h"
#include "pru_ctrl.h"
#include "resource_table_empty.h"

#include "pru_spidriver.h"

volatile register uint32_t __R30;
volatile register uint32_t __R31;

/* Mapping Constant table register to variable */
volatile far uint32_t SHARED_MEM_BASE __attribute__((cregister("PRU_SHAREDMEM", near), peripheral));
volatile far uint32_t PRU1_MEM_BASE __attribute__((cregister("PRU_DMEM_0_1", near), peripheral));

/* PRU-to-ARM interrupt */
#define PRU0_ARM_INTERRUPT (20+16)

// My device tree defines bit 8 as the output bit
#define CLK_OUT 8

// Command to AD7172
#define WRITE_ADCMODE_REG 0x01
#define READ_DATA_REG 0x44

#define SPIBUFFERSIZE 1000

#define DELAY_CNT 30

void pin_on(){
  __R30 = __R30 | (1 << CLK_OUT);
}

void pin_off(){
  __R30 = __R30 & ~(1 << CLK_OUT);
}


//====================================================
int main(void) {
  
  // command to AD7172
  uint32_t tx_buf[1];
  tx_buf[0] = READ_DATA_REG;
  
  pin_off();
  
  spi_writeread_continuous_start(tx_buf, 1, 0, 3, SPIBUFFERSIZE);
  while(1) {
    
    if(pru_read_word(0x00) == 0x00){
      pin_off();
    } else if (pru_read_word(0x00) == 0x01){
      pin_on();
    }
    
  }
  
  // We'll never get here
  return 0;
}
