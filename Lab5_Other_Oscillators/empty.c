#include "ti_msp_dl_config.h"
#include <stdio.h>
#include <stdint.h>

/* pin PINCM values — from friend's verified working code */
#define PB9  IOMUX_PINCM26
#define PB5  IOMUX_PINCM18    /* fixed: was PINCM22/53, friend uses PINCM18 */
#define PB11 IOMUX_PINCM28
#define PB21 IOMUX_PINCM49

#define TIMER_FREQ_HZ  32000000UL   /* 32 MHz */
#define TOTAL_CYCLES   10
#define SAMPLES        50

/* ── timer setup ─────────────────────────────────────────────────────────── */
void timerSetup(void)
{
    DL_Timer_enablePower(TIMG12);
    delay_cycles(16);

    DL_Timer_ClockConfig config;
    config.clockSel    = DL_TIMER_CLOCK_BUSCLK;
    config.divideRatio = DL_TIMER_CLOCK_DIVIDE_1;
    config.prescale    = 0;
    DL_Timer_setClockConfig(TIMG12, &config);

    DL_Timer_TimerConfig timerConfig;
    timerConfig.timerMode    = DL_TIMER_TIMER_MODE_PERIODIC_UP;
    timerConfig.period       = 0xFFFFFFFFU;
    timerConfig.startTimer   = DL_TIMER_START;
    timerConfig.genIntermInt = DL_TIMER_INTERM_INT_DISABLED;
    timerConfig.counterVal   = 0;
    DL_Timer_initTimerMode(TIMG12, &timerConfig);
}

/* ── pin setup helper ────────────────────────────────────────────────────── */
void pinSetup(uint32_t pincm, uint32_t resistor)
{
    DL_GPIO_initDigitalInputFeatures(
        pincm,
        DL_GPIO_INVERSION_DISABLE,
        resistor,
        DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE
    );
}

/* ── generic create cycles — works for any pin ───────────────────────────── */
void createCycles(uint32_t pincm, uint32_t gpiox_pin)
{
    uint8_t pin_state;

    pinSetup(pincm, DL_GPIO_RESISTOR_PULL_DOWN);
    delay_cycles(100);

    for (int i = 0; i < TOTAL_CYCLES; i++) {
        /* charge */
        pinSetup(pincm, DL_GPIO_RESISTOR_PULL_UP);
        do {
            pin_state = DL_GPIO_readPins(GPIOB, gpiox_pin) ? 1 : 0;
        } while (pin_state == 0);

        /* discharge */
        pinSetup(pincm, DL_GPIO_RESISTOR_PULL_DOWN);
        do {
            pin_state = DL_GPIO_readPins(GPIOB, gpiox_pin) ? 1 : 0;
        } while (pin_state == 1);
    }
}

/* ── frequency calculation ───────────────────────────────────────────────── */
float calculateFrequency(uint32_t total_time)
{
    if (total_time == 0U) return 0.0f;
    float cycle_time = (float)total_time / (TOTAL_CYCLES * (float)TIMER_FREQ_HZ);
    return 1.0f / cycle_time;
}

/* ── measure frequency for one pin ──────────────────────────────────────── */
float measureFrequency(uint32_t pincm, uint32_t gpiox_pin)
{
    uint32_t t0 = DL_Timer_getTimerCount(TIMG12);
    createCycles(pincm, gpiox_pin);
    uint32_t t1 = DL_Timer_getTimerCount(TIMG12);
    return calculateFrequency(t1 - t0);
}

/* ── main ────────────────────────────────────────────────────────────────── */
int main(void)
{
    SYSCFG_DL_init();
    DL_GPIO_enablePower(GPIOB);
    delay_cycles(16);
    timerSetup();

    float pb9_sum  = 0.0f;
    float pb5_sum  = 0.0f;
    float pb11_sum = 0.0f;
    float pb21_sum = 0.0f;

    for (int i = 0; i < SAMPLES; i++) {
        pb9_sum  += measureFrequency(PB9,  DL_GPIO_PIN_9);
        pb5_sum  += measureFrequency(PB5,  DL_GPIO_PIN_5);
        pb11_sum += measureFrequency(PB11, DL_GPIO_PIN_11);
        pb21_sum += measureFrequency(PB21, DL_GPIO_PIN_21);

        delay_cycles(TIMER_FREQ_HZ / 10U);   /* 100 ms */
    }

    printf("---Average Frequencies---\r\n");
    printf("PB9:  %7.2f Hz\r\n", pb9_sum  / SAMPLES);
    printf("PB5:  %7.2f Hz\r\n", pb5_sum  / SAMPLES);
    printf("PB11: %7.2f Hz\r\n", pb11_sum / SAMPLES);
    printf("PB21: %7.2f Hz\r\n", pb21_sum / SAMPLES);
    printf("Done\r\n");

    while (1) {}
}