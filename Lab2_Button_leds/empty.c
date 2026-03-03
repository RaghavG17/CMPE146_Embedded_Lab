#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "ti_msp_dl_config.h"

int main(void)
{
    SYSCFG_DL_init();

    /* Enable GPIOB */
    DL_GPIO_enablePower(GPIOB);

    /* ---- Blue LED (LED2) on PB22 ---- */
    DL_GPIO_initDigitalOutput(IOMUX_PINCM50);      // PB22
    DL_GPIO_enableOutput(GPIOB, DL_GPIO_PIN_22);
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_22);      // start OFF

    /* ---- S2 push button on PB21 (GPIOB DIO21) with pull-up ---- */
    DL_GPIO_initPeripheralInputFunctionFeatures(IOMUX_PINCM49,
        IOMUX_PINCM49_PF_GPIOB_DIO21,
        DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);

    /* ---- Bit-band/alias-byte pointers (no guessed hex addresses) ----
       PB22 is in DOUT23_20, byte2
       PB21 is in DIN23_20,  byte1
    */
    volatile uint8_t * const blue_led = ((volatile uint8_t *)&GPIOB->DOUT23_20) + 2u;
    volatile uint8_t * const s2_in    = ((volatile uint8_t *)&GPIOB->DIN23_20)  + 1u;

    /* Print alias addresses once (for lab report) */
    printf("PB22 (blue LED) alias = 0x%08lX\r\n", (unsigned long)blue_led);
    printf("PB21 (S2 input)  alias = 0x%08lX\r\n", (unsigned long)s2_in);

    bool led_on = false;
    uint8_t prev = 1u;   /* pull-up => not pressed reads as 1 */

    while (1) {
        uint8_t cur = *s2_in;   /* 1 = not pressed, 0 = pressed */

        /* toggle only at the moment S2 is pressed (falling edge: 1 -> 0) */
        if ((prev == 1u) && (cur == 0u)) {
            led_on = !led_on;
            *blue_led = led_on ? 1u : 0u;
        }

        prev = cur;
    }
}
