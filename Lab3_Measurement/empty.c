#include <stdio.h>
#include <stdint.h>
#include "ti_msp_dl_config.h"

#define QUEUE_SIZE 10

struct record {
    uint32_t timestamp;
    uint8_t state;
};

struct record queue[QUEUE_SIZE];

volatile int read_index = 0;
volatile int write_index = 0;

/* ================= ISR ================= */
void GROUP1_IRQHandler(void)
{
    uint32_t interrupt_status =
        DL_GPIO_getEnabledInterruptStatus(GPIOB, DL_GPIO_PIN_21);

    if ((interrupt_status & DL_GPIO_PIN_21) == DL_GPIO_PIN_21)
    {
        uint32_t ts = DL_Timer_getTimerCount(TIMG12);

        uint32_t pin = DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_21);
        uint8_t state = (pin & DL_GPIO_PIN_21) ? 1 : 0;

        int next = write_index + 1;
        if (next >= QUEUE_SIZE)
            next = 0;

        if (next == read_index)
        {
            printf("Queue full error\n");
        }
        else
        {
            queue[next].timestamp = ts;
            queue[next].state = state;
            write_index = next;
        }

        DL_GPIO_clearInterruptStatus(GPIOB, DL_GPIO_PIN_21);
    }
}

/* ================= MAIN ================= */
int main(void)
{
    SYSCFG_DL_init();

    /* ================= TIMER SETUP (32-bit TIMG12) ================= */
    DL_Timer_enablePower(TIMG12);

    DL_Timer_ClockConfig config;
    config.clockSel = DL_TIMER_CLOCK_BUSCLK;
    config.divideRatio = DL_TIMER_CLOCK_DIVIDE_1;
    config.prescale = 0;
    DL_Timer_setClockConfig(TIMG12, &config);

    DL_Timer_TimerConfig timerConfig;
    timerConfig.timerMode = DL_TIMER_TIMER_MODE_PERIODIC_UP;
    timerConfig.period = -1;              // 0xFFFFFFFF
    timerConfig.startTimer = DL_TIMER_START;
    timerConfig.genIntermInt = DL_TIMER_INTERM_INT_DISABLED;
    timerConfig.counterVal = 0;

    DL_Timer_initTimerMode(TIMG12, &timerConfig);
    /* =============================================================== */

    DL_GPIO_enablePower(GPIOB);

    /* Blue LED PB22 */
    DL_GPIO_initDigitalOutput(IOMUX_PINCM50);
    DL_GPIO_enableOutput(GPIOB, DL_GPIO_PIN_22);
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_22);

    /* Button PB21 pull-up */
    DL_GPIO_initPeripheralInputFunctionFeatures(
        IOMUX_PINCM49,
        IOMUX_PINCM49_PF_GPIOB_DIO21,
        DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE
    );

    /* Interrupt configuration */
    DL_GPIO_setUpperPinsPolarity(GPIOB, DL_GPIO_PIN_21_EDGE_RISE_FALL);
    DL_GPIO_clearInterruptStatus(GPIOB, DL_GPIO_PIN_21);
    DL_GPIO_enableInterrupt(GPIOB, DL_GPIO_PIN_21);
    NVIC_EnableIRQ(GPIOB_INT_IRQn);

    printf("Lab 3.2 running\n");

    int have_prev = 0;
    struct record prev;

    while (1)
    {
        /* Dummy delay 10 ms */
        delay_cycles(32000000 / 100);

        while (read_index != write_index)
        {
            int next = read_index + 1;
            if (next >= QUEUE_SIZE)
                next = 0;

            struct record r = queue[next];
            read_index = next;

            if (have_prev && prev.state == 0 && r.state == 1)
            {
                uint32_t diff = r.timestamp - prev.timestamp;

                /* 32 MHz → 32000 counts per ms */
                double ms = (double)diff / 32000.0;

                printf("Time = %.3f ms, read_index = %d\n", ms, read_index);
            }

            prev = r;
            have_prev = 1;
        }
    }
}