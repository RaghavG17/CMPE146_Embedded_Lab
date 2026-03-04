#include "ti_msp_dl_config.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* Lab: allocate one 64-bit word in flash (byte 0 = counter); aligned to sector */
_Alignas(DL_FLASHCTL_SECTOR_SIZE) const uint8_t nv_counter_word[8] =
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

/* ECC-uncorrected alias base for MSPM0G3507 (datasheet Table 8-5) */
#define FLASH_ECC_UNCORRECTED_BASE (0x00400000UL)
#define ECC_ALIAS_ADDR(addr)       ((uint32_t)(addr) + FLASH_ECC_UNCORRECTED_BASE)

/* Default LED pins (match your partner's board pins). If your project
   already defines LED macros in ti_msp_dl_config.h, you may replace these. */
#define GREEN_PORT   GPIOB
#define GREEN_PIN    DL_GPIO_PIN_27
#define RED_PORT     GPIOA
#define RED_PIN      DL_GPIO_PIN_0

static void blink_delay_half_second(void)
{
    /* 0.5s delay (toggle twice per second => 1 Hz blink) */
DL_Common_delayCycles(32000000);}

int main(void)
{
    SYSCFG_DL_init();

    /* address of our counter byte (byte 0 of the 8-byte word) */
    uint32_t counter_flash_addr = (uint32_t)&nv_counter_word[0];

    /* read via ECC-uncorrected alias to avoid ECC faults */
    volatile const uint8_t *pNoECC =
        (volatile const uint8_t *)ECC_ALIAS_ADDR(counter_flash_addr);

    uint8_t counter_val = pNoECC[0];

    /* Setup GPIO power & pins (minimal, per lab expectations) */
    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);

    /* Initialize LED pins as outputs (use simple init calls so it compiles) */
    DL_GPIO_initDigitalOutput(IOMUX_PINCM1);   /* PA0 (red) */
    DL_GPIO_initDigitalOutput(IOMUX_PINCM58);  /* PB27 (green) */

    DL_GPIO_enableOutput(GPIOA, RED_PIN);
    DL_GPIO_enableOutput(GPIOB, GREEN_PIN);

    /* Ensure LEDs start off */
    DL_GPIO_clearPins(RED_PORT, RED_PIN);
    DL_GPIO_clearPins(GREEN_PORT, GREEN_PIN);

    /* If bits 0..2 are already all zero => expired -> red ON forever */
    if ((counter_val & 0x07u) == 0u) {
        DL_GPIO_setPins(RED_PORT, RED_PIN);
        while (1) { /* locked on red */ }
    }

    /* Not expired: clear the next available low-order set bit (0, then 1, then 2) */
    uint8_t new_val = counter_val;
    if (counter_val & (1u << 0)) {
        new_val = (uint8_t)(counter_val & (uint8_t)~(1u << 0));
    } else if (counter_val & (1u << 1)) {
        new_val = (uint8_t)(counter_val & (uint8_t)~(1u << 1));
    } else {
        new_val = (uint8_t)(counter_val & (uint8_t)~(1u << 2));
    }

    /* Prepare flash and program the byte (no erase) per lab */
    DL_FlashCTL_executeClearStatus(FLASHCTL);
    DL_FlashCTL_unprotectSector(FLASHCTL, counter_flash_addr, DL_FLASHCTL_REGION_SELECT_MAIN);

    DL_FLASHCTL_COMMAND_STATUS st =
        DL_FlashCTL_programMemoryFromRAM8(FLASHCTL, counter_flash_addr, &new_val);

    /* If programming failed -> turn red on and halt (lab behaviour) */
    if (st == DL_FLASHCTL_COMMAND_STATUS_FAILED) {
        DL_FLASHCTL_FAIL_TYPE reason = DL_FlashCTL_getFailureStatus(FLASHCTL);
        printf("Flash program failed, reason: %d\n", (int)reason);
        DL_GPIO_setPins(RED_PORT, RED_PIN);
        while (1) { }
    }

    /* Success -> blink green at 1 Hz (toggle every 0.5s) */
    while (1) {
        DL_GPIO_togglePins(GREEN_PORT, GREEN_PIN);
        blink_delay_half_second();
    }
}