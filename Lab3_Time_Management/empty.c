#include "ti_msp_dl_config.h"
#include <stdio.h>

#define PB21_ALIAS  0x400A3315  // Alias address for S2 (user button)
#define TIMER_FREQ_HZ 3200000

int main(void)
{
    SYSCFG_DL_init();

    // Enabling power to GPIOB (port b)
    DL_GPIO_enablePower(GPIOB);
    // Configuring PB21 (user button) as input with the addition of a pull up resistor at the pin
    DL_GPIO_initPeripheralInputFunctionFeatures(IOMUX_PINCM49,
        IOMUX_PINCM49_PF_GPIOB_DIO21,
        DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_UP,       // Pull up resistor
        DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);

    // Creating a byte pointer to bit band alias
    volatile uint8_t *USER_BUTTON_PTR = (volatile uint8_t *)PB21_ALIAS;

    // Printing address of the PB22 alias and PB21 (S2) alias for validation
    printf("Bit band alias address for S2 (user button): 0x%08X\n", (unsigned int)USER_BUTTON_PTR);

    // Getting status of the button
    uint32_t STATUS_PB = DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_21);

    // Set up to use the 32 bit counter TIMG12
    DL_Timer_enablePower(TIMG12);
    DL_Timer_ClockConfig config;
    config.clockSel = DL_TIMER_CLOCK_BUSCLK;
    config.divideRatio = DL_TIMER_CLOCK_DIVIDE_1;
    config.prescale = 0;
    DL_Timer_setClockConfig(TIMG12, &config);
    DL_Timer_TimerConfig timerConfig;
    timerConfig.timerMode = DL_TIMER_TIMER_MODE_PERIODIC_UP;
    timerConfig.period = -1;
    timerConfig.startTimer = DL_TIMER_START;
    timerConfig.genIntermInt = DL_TIMER_INTERM_INT_DISABLED;
    timerConfig.counterVal = 0;
    DL_Timer_initTimerMode(TIMG12, &timerConfig);

    uint8_t PREVIOUS_STATE = 0;     // 1 = pressed, 0 = released
    uint32_t PUSHED_VALUE;          // Time when button is pushed
    uint32_t RELEASED_VALUE;        // Time when button is released
    uint32_t FINAL_VALUE;           // Calculated time of how long button is pressed
    uint32_t TIME_MS;               // Time in ms

    while (1) {
        // Outputting messages if button is pressed/released
        if(!*USER_BUTTON_PTR && !PREVIOUS_STATE){
            printf("Button Pressed\n");
            PUSHED_VALUE = DL_Timer_getTimerCount(TIMG12);
            PREVIOUS_STATE = 1;
        }else if(*USER_BUTTON_PTR && PREVIOUS_STATE){
            printf("Button released\n");
            RELEASED_VALUE = DL_Timer_getTimerCount(TIMG12);
            FINAL_VALUE = RELEASED_VALUE - PUSHED_VALUE;
            TIME_MS = (uint32_t)(((uint64_t)FINAL_VALUE * 100) / TIMER_FREQ_HZ);
            printf("Time: %u ms\n", TIME_MS);
            PREVIOUS_STATE = 0;
        }
        delay_cycles(500000);
    }

}
