#include "ti_msp_dl_config.h"
#include <stdio.h>

// Global LED State
volatile uint8_t LED_STATE = 0; // Volatile since it is accessed in main and interrupt service routine (ISR)
// Global REVIOUS_STATE
volatile uint8_t PREVIOUS_STATE = 0;

// GPIO Group 1 ISR that handles interrupts from S2 button (PB21)
void GROUP1_IRQHandler(void){
    // Get pending interrupt status for GPIOB
    uint32_t INTERRUPT_STATUS = DL_GPIO_getEnabledInterruptStatus(GPIOB, DL_GPIO_PIN_21);
    // Check if the interrupt came from PB21
    if(INTERRUPT_STATUS & DL_GPIO_PIN_21){
        // Read current state of PB21: 0 = pressed, 1 = released
        uint8_t CURRENT_STATE = DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_21);
        //Check button state
        // !0 && 0 = 0 (idle, don't toggle)
        // !0 && 1 = 1 (pressed, toggle)
        // !1 && 0 = 0 (released, toggle)
        // !1 && 1 = 0 (holding, don't toggle)
        if(!PREVIOUS_STATE && ((CURRENT_STATE & DL_GPIO_PIN_21) == 0)){
            // Button pressed so toggle
            LED_STATE ^= 1;
            // Set new state to LED
            if(LED_STATE){
                DL_GPIO_setPins(GPIOB, DL_GPIO_PIN_22);
            }else{
                DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_22);
            }
        }
        PREVIOUS_STATE = CURRENT_STATE;
        // Clearing the interrupt flag for PB21
        DL_GPIO_clearInterruptStatus(GPIOB, DL_GPIO_PIN_21);
    }
}


int main(void)
{
    SYSCFG_DL_init();

    // Enabling power to GPIOB (port b)
    DL_GPIO_enablePower(GPIOB);
    // Configuring PB22 (blue LED) as the digital output
    DL_GPIO_initDigitalOutput(IOMUX_PINCM50);   // IOMUX_PINCM50 maps to the blue LED pin (found on datasheet section 6.2)
    // Configuring PB21 (user button) as input with the addition of a pull up resistor at the pin
    DL_GPIO_initPeripheralInputFunctionFeatures(IOMUX_PINCM49,
        IOMUX_PINCM49_PF_GPIOB_DIO21,
        DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_UP,       // Pull up resistor
        DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    // Enabling output driver for PB22
    DL_GPIO_enableOutput(GPIOB, DL_GPIO_PIN_22);
    // Initializing all LEDs to an off state
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_22);   // Blue LED
    DL_GPIO_clearPins(GPIOA, DL_GPIO_PIN_0);   // Red LED
    DL_GPIO_clearPins(GPIOB, DL_GPIO_PIN_27);   // Green LED

    // Interrupt configuration
    // Trigger interrupt on both rising and falling edges
    DL_GPIO_setUpperPinsPolarity(GPIOB, DL_GPIO_PIN_21_EDGE_RISE_FALL);
    // Clearing any pending interrupts
    DL_GPIO_clearInterruptStatus(GPIOB, DL_GPIO_PIN_21);
    // Enable interrupt from PB21
    DL_GPIO_enableInterrupt(GPIOB, DL_GPIO_PIN_21);
    // Enable GPIOB interrupt in NVIC
    NVIC_EnableIRQ(GPIOB_INT_IRQn);

    while (1) {

    }

}
