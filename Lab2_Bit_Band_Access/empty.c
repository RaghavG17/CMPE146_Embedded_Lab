#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "ti_msp_dl_config.h"

#define GPIOB_BASE            (0x400A2000u)   // GPIO1 / GPIOB base (your value)
#define DOUT23_20_OFFSET      (0x400A3216u)       // <-- fill from TRM Table 9-2

int main(void)
{
    SYSCFG_DL_init();

    /* same DriverLib setup as 1.1 / 1.2 */
    DL_GPIO_enablePower(GPIOB);

    DL_GPIO_initDigitalOutput(IOMUX_PINCM57); // PB26 red
    DL_GPIO_initDigitalOutput(IOMUX_PINCM58); // PB27 green
    DL_GPIO_initDigitalOutput(IOMUX_PINCM50); // PB22 blue

    DL_GPIO_enableOutput(GPIOB, DL_GPIO_PIN_26);
    DL_GPIO_enableOutput(GPIOB, DL_GPIO_PIN_27);
    DL_GPIO_enableOutput(GPIOB, DL_GPIO_PIN_22);

    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_22 | DL_GPIO_PIN_26 | DL_GPIO_PIN_27);

    /* ---- bit-band/alias byte pointer for PB22 ----
       PB22 is in DOUT23_20, byte offset 2 (byte2 -> bit22)
    */
    volatile uint8_t * const blue_alias =
    ((volatile uint8_t *)&GPIOB->DOUT23_20) + 2u;

printf("PB22 alias address = 0x%08lX\r\n", (unsigned long)(uintptr_t)blue_alias);

    bool blue_on = false;

    while (1) {
        if (blue_on) {
            *blue_alias = 0;   // turn off PB22
            blue_on = false;
        } else {
            *blue_alias = 1;   // turn on PB22
            blue_on = true;
        }

        delay_cycles(32000000);
    }
}

