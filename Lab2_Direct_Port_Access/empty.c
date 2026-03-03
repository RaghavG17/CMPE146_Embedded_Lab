#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "ti_msp_dl_config.h"   /* DriverLib / project config (pulls in device headers) */

/* bit mask for PB22 (blue LED) */
#define BLUE_MASK (1u << 22)

int main(void)
{
    SYSCFG_DL_init();

    /* Keep DriverLib setup from Exercise 1.1 */
    DL_GPIO_enablePower(GPIOB);
    DL_GPIO_initDigitalOutput(IOMUX_PINCM57); /* PB26 red */
    DL_GPIO_initDigitalOutput(IOMUX_PINCM58); /* PB27 green */
    DL_GPIO_initDigitalOutput(IOMUX_PINCM50); /* PB22 blue */
    DL_GPIO_enableOutput(GPIOB, DL_GPIO_PIN_26);
    DL_GPIO_enableOutput(GPIOB, DL_GPIO_PIN_27);
    DL_GPIO_enableOutput(GPIOB, DL_GPIO_PIN_22);

    /* start from known state: turn off LEDs */
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_22 | DL_GPIO_PIN_26 | DL_GPIO_PIN_27);

    /* Direct register pointers for Exercise 1.2 (volatile) */
    volatile uint32_t * const gpio_din  = &GPIOB->DIN31_0;
    volatile uint32_t * const gpio_dout = &GPIOB->DOUT31_0;

    bool blue_on = false;

while (1)
{
    uint32_t port_state = *gpio_din;   // read all 32 bits

    if (blue_on) {
        port_state &= ~(1u << 22);     // clear PB22
        blue_on = false;
    } else {
        port_state |= (1u << 22);      // set PB22
        blue_on = true;
    }

    *gpio_dout = port_state;            // write full port back

    delay_cycles(32000000);
}

}
