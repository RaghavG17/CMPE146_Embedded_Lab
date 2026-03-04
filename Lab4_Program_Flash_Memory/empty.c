#include "ti_msp_dl_config.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* Reserve one full 64-bit word so the printed 8 bytes are all ours */
_Alignas(DL_FLASHCTL_SECTOR_SIZE) const uint8_t flash_data[8] =
    {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

/* MSPM0G3507 datasheet: Flash ECC Uncorrected starts at 0x0040.0000 */
#define FLASH_ECC_UNCORRECTED_BASE (0x00400000UL)

int main(void)
{
    SYSCFG_DL_init();

    uint32_t flash_addr = (uint32_t)&flash_data[0];

    /* Unprotect the sector that contains flash_data[] */
    DL_FlashCTL_unprotectSector(FLASHCTL, flash_addr, DL_FLASHCTL_REGION_SELECT_MAIN);

    uint8_t value = 0xFF;

    for (uint8_t bit = 0; bit < 8; bit++) {

        /* Clear one more bit each time (1 -> 0 only) */
        value = (uint8_t)(value & (uint8_t)~(1U << bit));

        /* Clear flash status before command (needed for repeated programming) */
        DL_FlashCTL_executeClearStatus(FLASHCTL);

        /* Ensure sector is unprotected before programming */
        DL_FlashCTL_unprotectSector(FLASHCTL, flash_addr, DL_FLASHCTL_REGION_SELECT_MAIN);

        /* Program one byte */
        DL_FlashCTL_programMemoryFromRAM8(FLASHCTL, flash_addr, &value);

        /* Read 8-byte word from ECC-uncorrected alias region */
        uint32_t alias_addr = FLASH_ECC_UNCORRECTED_BASE + (flash_addr & 0x000FFFFFUL);
        volatile const uint8_t *p8 = (volatile const uint8_t *)alias_addr;

        printf("After programming Bit %u: ", bit);
        for (uint8_t i = 0; i < 8; i++) {
            printf("%02X ", p8[i]);
        }
        printf("\n");
    }

    while (1) {
    }
}