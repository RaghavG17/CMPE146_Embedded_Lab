#include "ti_msp_dl_config.h"
#include <stdio.h>

// PB9 pin configuration
#define PB9 IOMUX_PINCM26    // Value found in Datasheet
#define TIMER_FREQ_HZ 3200000
#define TOTAL_CYCLES 10

// Function to create a number of charge/discharge
void createCycles(void){    
    // Start with pin discharged
    DL_GPIO_initDigitalInputFeatures(
        PB9,
        DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_DOWN,
        DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE
    );
    
    // Small delay to discharging at beginning
    delay_cycles(100);

    uint8_t pin_state;
    
    for(int i = 0; i < TOTAL_CYCLES; i++){
        // Charging phase: Enable pull up
        DL_GPIO_initDigitalInputFeatures(
            PB9,
            DL_GPIO_INVERSION_DISABLE,
            DL_GPIO_RESISTOR_PULL_UP,
            DL_GPIO_HYSTERESIS_DISABLE,
            DL_GPIO_WAKEUP_DISABLE
        );
        
        // Wait for the pin to actually reach its high state
        do{
            pin_state = DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_9) ? 1 : 0;
        }while(pin_state == 0);  // Wait until pin is high
        
        // Discharging phase: Enable pull down
        DL_GPIO_initDigitalInputFeatures(
            PB9,
            DL_GPIO_INVERSION_DISABLE,
            DL_GPIO_RESISTOR_PULL_DOWN,
            DL_GPIO_HYSTERESIS_DISABLE,
            DL_GPIO_WAKEUP_DISABLE
        );
        
        // Wait for the pin to actually reach its low state
        do{
            pin_state = DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_9) ? 1 : 0;
        }while(pin_state == 1);  // Wait until pin is low
    }
}

float calculateFrequency(uint32_t total_time){
    // frequency in hz = 1/t where t = time in seconds for how long it takes to complete the cycles
    float cycle_time = (float)total_time / (TOTAL_CYCLES * TIMER_FREQ_HZ);
    return 1.0f / cycle_time;
}

int main(void)
{
    SYSCFG_DL_init();

    // Enable power to GPIOB
    DL_GPIO_enablePower(GPIOB);

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

    uint32_t cycles_start;
    uint32_t cycles_end;
    uint32_t time_difference;
    float frequency_value;
    int i = 0;

    for(i = 0; i < 50; i++){
        // Getting time to complete 10 cycles
        cycles_start = DL_Timer_getTimerCount(TIMG12);
        createCycles();
        cycles_end = DL_Timer_getTimerCount(TIMG12);
        time_difference = cycles_end - cycles_start;
        frequency_value = calculateFrequency(time_difference);
        printf("Iteration: %i --- Frequency: %7.2f Hz\n", i+1, frequency_value);
        // 100 ms delay
        delay_cycles(TIMER_FREQ_HZ / 10);
    }
    printf("Done\n");

    while (1) {
    }

}