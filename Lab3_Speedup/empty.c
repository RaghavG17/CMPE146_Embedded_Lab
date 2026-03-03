#include "ti_msp_dl_config.h"
#include <stdio.h>
#include <stdlib.h>

#define CRC32_SEED 0xFFFFFFFF
#define TIMER_FREQ_HZ 3200000

// Array of 10240 bytes (10KB) for testing
static uint8_t myData[10240];

uint32_t compute_simple_checksum(uint8_t *data, uint32_t length){
    uint32_t sum = 0;       // Holds accumulated sum
    uint32_t i;
    uint8_t byte_index = 0;     // Tracking which byte position from 0 to 3 to add to
    // Looping through every byte in the input data array
    for(i = 0; i < length; i++){
        // Add current byte to the repsective byte position in the 32 bit sum
        // Shift left by (byte_index * 8) bits to place it in the correct byte lane
        // byte_index = 0 -> shift 0 bits then place in lsb (Byte 0)
        // byte_index = 1 -> shift 8 bits then place in second byte (Byte 1)
        // byte_index = 2 -> shift 16 bits then place in third byte (Byte 2)
        // byte_index = 3 -> shift 24 bits then place in msb (Byte 3)
        sum += ((uint32_t)data[i] << (byte_index * 8));
        byte_index++;
        // Wrapping around after the 3rd byte
        if(byte_index > 3){
            byte_index = 0;
        }
    }

    // Reversing bits
    uint32_t reversed_bits = 0;
    for(i = 0; i < 32; i++){
        // (sum >> i): shift bit i down to position 0
        // & 1: mask to isolate the single bit (0 or 1)
        // Place it in the reversed position (31 - i)
        // << (31 - i): shift the isolated bit to its new mirrored position
        // OR-assign to build up the reversed value bit by bit
        reversed_bits |= ((sum >> i) & 1) << (31 - i);
    }
    return reversed_bits;
}

// Standard software calculation of CRC32
static uint32_t calculateCRC32(uint8_t* data, uint32_t length){
    const uint32_t crc32_poly = 0xEDB88320;
    uint32_t ii, jj, byte, crc, mask;
    crc = CRC32_SEED;

    for(ii = 0; ii < length; ii++){
        byte = data[ii];
        crc = crc ^ byte;

        for(jj = 0; jj < 8; jj++){
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (crc32_poly & mask);
        }
    }
    return crc;
}

// Hardware calculation of CRC32
uint32_t hardware_CRC_calculation(uint8_t* data, uint32_t length){
    // Resetting CRC accelerator and set the seed
    DL_CRC_setSeed32(CRC, CRC32_SEED);
    // Setting each byte to the CRC accelerator
    for(uint32_t i = 0; i < length; i++){
        DL_CRC_feedData8(CRC, data[i]);
    }
    // Returning the final result of CRC
    return DL_CRC_getResult32(CRC);
}



int main(void)
{
    SYSCFG_DL_init();
    
    // Seeding random number generator
    srand(12345);
    // Filling array with random bytes
    int i = 0;
    for(i = 0; i < 10240; i++){
        myData[i] = rand();
    }

    // CRC Setup
    DL_CRC_reset(CRC);
    DL_CRC_enablePower(CRC);
    delay_cycles(POWER_STARTUP_DELAY);
    DL_CRC_init(CRC, DL_CRC_32_POLYNOMIAL, DL_CRC_BIT_REVERSED, DL_CRC_INPUT_ENDIANESS_LITTLE_ENDIAN, DL_CRC_OUTPUT_BYTESWAP_DISABLED);
    DL_CRC_setSeed32(CRC, CRC32_SEED);

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

    // Printing the first four bytes for data verification
    printf("First four bytes: 0x%02X, 0x%02X, 0x%02X, 0x%02X\n", myData[0], myData[1], myData[2], myData[3]);
    // Printing the last four bytes for data verification
    printf("Last four bytes: 0x%02X, 0x%02X, 0x%02X, 0x%02X\n", myData[10236], myData[10237], myData[10238], myData[10239]);
    
    // Setting timer variables
    uint32_t start_checksum;
    uint32_t end_checksum;
    uint32_t time_difference;
    uint32_t final_time;
    
    // Computing simple checksum
    start_checksum = DL_Timer_getTimerCount(TIMG12);
    uint32_t simple_checksum_value = compute_simple_checksum(myData, 10240);
    end_checksum = DL_Timer_getTimerCount(TIMG12);
    time_difference = end_checksum - start_checksum;
    final_time = (uint32_t)(((uint64_t)time_difference * 100000) / TIMER_FREQ_HZ);
    printf("Simple Checksum: 0x%08X\n", simple_checksum_value);
    printf("Time elapsed: %u microseconds\n", final_time);

    // Computing Software CRC32
    start_checksum = DL_Timer_getTimerCount(TIMG12);
    uint32_t software_CRC = calculateCRC32(myData, 10240);
    end_checksum = DL_Timer_getTimerCount(TIMG12);
    time_difference = end_checksum - start_checksum;
    final_time = (uint32_t)(((uint64_t)time_difference * 100000) / TIMER_FREQ_HZ);
    printf("Software CRC Checksum: 0x%08X\n", software_CRC);
    printf("Time elapsed: %u microseconds\n", final_time);
    float sw_time = final_time;

    // Computing Hardware CRC32
    start_checksum = DL_Timer_getTimerCount(TIMG12);
    uint32_t hardware_CRC = hardware_CRC_calculation(myData, 10240);
    end_checksum = DL_Timer_getTimerCount(TIMG12);
    time_difference = end_checksum - start_checksum;
    final_time = (uint32_t)(((uint64_t)time_difference * 100000) / TIMER_FREQ_HZ);
    printf("Hardware CRC Checksum: 0x%08X\n", hardware_CRC);
    printf("Time elapsed: %u microseconds\n", final_time);
    float hw_time = final_time;

    // Calculating speedup: software CRC / hardware CRC
    float speedup = sw_time / hw_time;
    printf("Speedup (Software time / Hardware time): %.2f\n", speedup);

    while (1) {
    }
}
