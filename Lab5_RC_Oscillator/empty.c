#include "ti_msp_dl_config.h"
#include <stdio.h>
#include <stdint.h>

/*
 * Exercise 1: RC oscillator on PB9
 *
 * PB9 on MSPM0G3507 uses:
 *   - Port:  GPIOB
 *   - Pin:   DL_GPIO_PIN_9
 *   - IOMUX: IOMUX_PINCM26
 */
#define RC_OSC_PORT   (GPIOB)
#define RC_OSC_PIN    (DL_GPIO_PIN_9)
#define RC_OSC_IOMUX  (IOMUX_PINCM26)

static inline void rcOsc_usePullUp(void)
{
    DL_GPIO_initDigitalInputFeatures(
        RC_OSC_IOMUX,
        DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_ENABLE,
        DL_GPIO_WAKEUP_DISABLE
    );
}

static inline void rcOsc_usePullDown(void)
{
    DL_GPIO_initDigitalInputFeatures(
        RC_OSC_IOMUX,
        DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_DOWN,
        DL_GPIO_HYSTERESIS_ENABLE,
        DL_GPIO_WAKEUP_DISABLE
    );
}

static inline uint32_t rcOsc_read(void)
{
    return DL_GPIO_readPins(RC_OSC_PORT, RC_OSC_PIN);
}

int main(void)
{
    uint32_t i;

    SYSCFG_DL_init();

    /*
     * Start by discharging C so the pin is definitely low.
     * Do not print here; the lab only needs the binary stream
     * during the 100 oscillation cycles.
     */
    rcOsc_usePullDown();
    while (rcOsc_read() != 0U) {
        ;
    }

    /*
     * 100 oscillation cycles:
     *   charge to High  -> print 1
     *   discharge to Low -> print 0
     *
     * No newline inside the loop.
     */
    for (i = 0; i < 100U; i++) {

        /* Charge through pull-up until pin becomes High */
        rcOsc_usePullUp();
        while (rcOsc_read() == 0U) {
            ;
        }
        printf("1");

        /* Discharge through pull-down until pin becomes Low */
        rcOsc_usePullDown();
        while (rcOsc_read() != 0U) {
            ;
        }
        printf("0");
    }

    /* One newline after the 100 cycles */
    printf("\n");

    while (1) {
        ;
    }
}