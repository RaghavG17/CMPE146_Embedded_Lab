#include "ti_msp_dl_config.h"
#include <stdio.h>
#include <stdint.h>

#define PB9_PINCM       IOMUX_PINCM26
#define TIMER_FREQ_HZ   32000000UL      /* 32 MHz */
#define TOTAL_CYCLES    10
#define CAL_SAMPLES     8

/* LED1 = PA0 on MSPM0G3507 LaunchPad, configured directly like friend's code */
#define LED_PORT        GPIOA
#define LED_PIN         DL_GPIO_PIN_0
#define LED_PINCM       IOMUX_PINCM1

/* ── charge/discharge cycles ─────────────────────────────────────────────── */
void createCycles(void)
{
    uint8_t pin_state;

    DL_GPIO_initDigitalInputFeatures(
        PB9_PINCM,
        DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_DOWN,
        DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE
    );
    delay_cycles(100);

    for (int i = 0; i < TOTAL_CYCLES; i++) {
        DL_GPIO_initDigitalInputFeatures(
            PB9_PINCM,
            DL_GPIO_INVERSION_DISABLE,
            DL_GPIO_RESISTOR_PULL_UP,
            DL_GPIO_HYSTERESIS_DISABLE,
            DL_GPIO_WAKEUP_DISABLE
        );
        do {
            pin_state = (DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_9) != 0U) ? 1U : 0U;
        } while (pin_state == 0U);

        DL_GPIO_initDigitalInputFeatures(
            PB9_PINCM,
            DL_GPIO_INVERSION_DISABLE,
            DL_GPIO_RESISTOR_PULL_DOWN,
            DL_GPIO_HYSTERESIS_DISABLE,
            DL_GPIO_WAKEUP_DISABLE
        );
        do {
            pin_state = (DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_9) != 0U) ? 1U : 0U;
        } while (pin_state == 1U);
    }
}

/* ── frequency calculation ───────────────────────────────────────────────── */
float calculateFrequency(uint32_t total_time)
{
    if (total_time == 0U) return 0.0f;
    return ((float)TOTAL_CYCLES * (float)TIMER_FREQ_HZ) / (float)total_time;
}

/* ── single frequency measurement ───────────────────────────────────────── */
float measureFrequency(void)
{
    uint32_t t0 = DL_Timer_getTimerCount(TIMG12);
    createCycles();
    uint32_t t1 = DL_Timer_getTimerCount(TIMG12);
    return calculateFrequency(t1 - t0);
}

/* ── main ────────────────────────────────────────────────────────────────── */
int main(void)
{
    SYSCFG_DL_init();

    /* init LED pin directly — same approach as friend, no SysConfig macros */
    DL_GPIO_initDigitalOutput(LED_PINCM);
    DL_GPIO_enableOutput(LED_PORT, LED_PIN);
    DL_GPIO_clearPins(LED_PORT, LED_PIN);    /* LED off initially */

    /* TIMG12 init entirely in code */
    DL_Timer_enablePower(TIMG12);
    delay_cycles(16);

    DL_Timer_ClockConfig clkCfg;
    clkCfg.clockSel    = DL_TIMER_CLOCK_BUSCLK;
    clkCfg.divideRatio = DL_TIMER_CLOCK_DIVIDE_1;
    clkCfg.prescale    = 0;
    DL_Timer_setClockConfig(TIMG12, &clkCfg);

    DL_Timer_TimerConfig tmrCfg;
    tmrCfg.timerMode    = DL_TIMER_TIMER_MODE_PERIODIC_UP;
    tmrCfg.period       = 0xFFFFFFFFU;
    tmrCfg.startTimer   = DL_TIMER_START;
    tmrCfg.genIntermInt = DL_TIMER_INTERM_INT_DISABLED;
    tmrCfg.counterVal   = 0;
    DL_Timer_initTimerMode(TIMG12, &tmrCfg);

    /* calibration — do not touch pin during this phase */
    delay_cycles(TIMER_FREQ_HZ / 10U);

    float sum = 0.0f;
    for (int i = 0; i < CAL_SAMPLES; i++) {
        sum += measureFrequency();
        delay_cycles(TIMER_FREQ_HZ / 20U);
    }

    float baseline  = sum / CAL_SAMPLES;
    float threshold = baseline * 0.80f;

    printf("baseline  = %.2f Hz\r\n", baseline);
    printf("threshold = %.2f Hz\r\n", threshold);
    printf("Starting control loop...\r\n");

    /* control loop — no printf, runs forever */
    int lastTouched = 0;

    while (1) {
        float f       = measureFrequency();
        int   touched = (f < threshold) ? 1 : 0;

        /* toggle only on rising edge — not while finger held down */
        if (touched && !lastTouched) {
            DL_GPIO_togglePins(LED_PORT, LED_PIN);
        }

        lastTouched = touched;
    }
}