#include "ti_msp_dl_config.h"
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#define PB22_ALIAS  0x400A3216u  // blue LED (LED2) alias
#define PA0_ALIAS   0x400A1200u  // red LED (LED1) alias

int main(void)
{
    SYSCFG_DL_init();

    DL_GPIO_enablePower(GPIOB);
    DL_GPIO_enablePower(GPIOA);

    // PB22 as output
    DL_GPIO_initDigitalOutput(IOMUX_PINCM50);

    // PA0 as output with inversion (LED1 active-low)
    DL_GPIO_initDigitalOutputFeatures(IOMUX_PINCM1,
            DL_GPIO_INVERSION_ENABLE,
            DL_GPIO_RESISTOR_NONE,          // output pin: no pull needed
            DL_GPIO_DRIVE_STRENGTH_LOW,
            DL_GPIO_HIZ_DISABLE);

    DL_GPIO_enableOutput(GPIOB, DL_GPIO_PIN_22);
    DL_GPIO_enableOutput(GPIOA, DL_GPIO_PIN_0);

    // Known off state (with inversion enabled, clear = OFF is fine)
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_22);
    DL_GPIO_clearPins(GPIOA, DL_GPIO_PIN_0);

    volatile uint8_t *BLUE_LED_PTR = (volatile uint8_t *)PB22_ALIAS;
    volatile uint8_t *RED_LED_PTR  = (volatile uint8_t *)PA0_ALIAS;

    printf("Bit band alias address for blue LED: 0x%08" PRIXPTR "\r\n", (uintptr_t)BLUE_LED_PTR);
    printf("Bit band alias address for red  LED: 0x%08" PRIXPTR "\r\n", (uintptr_t)RED_LED_PTR);

    uint8_t LED_STATE = 0;

    while (1) {
        if (LED_STATE == 0) {
            *BLUE_LED_PTR = 1;
            *RED_LED_PTR  = 1;
            LED_STATE = 1;
        } else {
            *BLUE_LED_PTR = 0;
            *RED_LED_PTR  = 0;
            LED_STATE = 0;
        }
        delay_cycles(32000000);
    }
}
