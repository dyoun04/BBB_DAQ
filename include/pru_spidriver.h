#ifndef PRU_SPIDRIVER_H
#define PRU_SPIDRIVER_H

#include <stdint.h>

#define RAMOFFSET 0x80
volatile far uint32_t MEM_BASE __attribute__((cregister("PRU_SHAREDMEM", near), peripheral));



void spi_reset_cmd(void);

// Low level fcns for internal use.
uint32_t pru_read_word(uint32_t offset);
void pru_read_block(uint32_t offset, uint32_t count, uint32_t *buffer);
void pru_write_word(uint32_t offset, uint32_t value);

// High level fcns.  Call these from external files.
uint32_t spi_write_cmd(uint32_t *data, int byte_cnt);
uint32_t spi_writeread_continuous_start(uint32_t *txdata, int txcnt, uint32_t rxoffset, int rxcnt, int ncnv);
uint32_t spi_writeread_continuous_waitstart(uint32_t *txdata, int txcnt, uint32_t rxoffset, int rxcnt, int ncnv);

#endif
