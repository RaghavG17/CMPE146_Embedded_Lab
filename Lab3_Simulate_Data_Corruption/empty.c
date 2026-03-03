#include "ti_msp_dl_config.h"
#include <stdio.h>
#include <stdlib.h>

#define CRC32_SEED 0xFFFFFFFF
#define TIMER_FREQ_HZ 3200000

// Array of 10240 bytes (10KB) for testing
static uint8_t myData[10240];
static uint8_t corruptedData[10240];       // Data to be used to corrupt 1 then 2 bits

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

    // Copying data from myData into corruptedData array to be corrupted for comparisons
    for(i = 0; i < 10240; i++){
        corruptedData[i] = myData[i];
    }

    // CRC Setup
    DL_CRC_reset(CRC);
    DL_CRC_enablePower(CRC);
    delay_cycles(POWER_STARTUP_DELAY);
    DL_CRC_init(CRC, DL_CRC_32_POLYNOMIAL, DL_CRC_BIT_REVERSED, DL_CRC_INPUT_ENDIANESS_LITTLE_ENDIAN, DL_CRC_OUTPUT_BYTESWAP_DISABLED);
    DL_CRC_setSeed32(CRC, CRC32_SEED);

    // Printing the first four bytes for data verification
    printf("First four bytes: 0x%02X, 0x%02X, 0x%02X, 0x%02X\n", myData[0], myData[1], myData[2], myData[3]);
    // Printing the last four bytes for data verification
    printf("Last four bytes: 0x%02X, 0x%02X, 0x%02X, 0x%02X\n", myData[10236], myData[10237], myData[10238], myData[10239]);
    
    // Setting variables to hold simple checksum and hardware crc checksum
    uint32_t simple_checksum;
    uint32_t hardware_CRC_checksum;

    // ==============Set 1: Original data without any changes==============
    printf("==============Set 1: Original Data with No Corruption==============\n");
    simple_checksum = compute_simple_checksum(myData, 10240);
    hardware_CRC_checksum = hardware_CRC_calculation(myData, 10240);
    printf("Simple Checksum: 0x%08X\n", simple_checksum);
    printf("Hardware CRC Checksum: 0x%08X\n", hardware_CRC_checksum);

    // ==============Set 2: Original with one bit "corrupted"==============
    printf("==============Set 2: Original with One Bit 'Corrupted'==============\n");
    printf("Before Corruption at bit 0 of byte 2496:\n");
    printf("Byte at 2496 = 0x%02X\n", corruptedData[2496]);
    simple_checksum = compute_simple_checksum(corruptedData, 10240);
    hardware_CRC_checksum = hardware_CRC_calculation(corruptedData, 10240);
    printf("Simple Checksum: 0x%08X\n", simple_checksum);
    printf("Hardware CRC Checksum: 0x%08X\n", hardware_CRC_checksum);
    // Corrupting bit at position 0 of byte 2496
    corruptedData[2496] ^= 1;      // Flips LSB
    printf("After Corruption:\n");
    printf("Byte at 2496 = 0x%02X\n", corruptedData[2496]);
    simple_checksum = compute_simple_checksum(corruptedData, 10240);
    hardware_CRC_checksum = hardware_CRC_calculation(corruptedData, 10240);
    printf("Simple Checksum: 0x%08X\n", simple_checksum);
    printf("Hardware CRC Checksum: 0x%08X\n", hardware_CRC_checksum);

    // ==============Set 3: Original with two bits "corrupted"==============
    printf("==============Set 3: Original with Two Bits 'Corrupted'==============\n");
    printf("Before Corruption at bit 0 of bytes 2496 and 8864:\n");
    printf("Byte at 8864 = 0x%02X\n", corruptedData[8864]);
    simple_checksum = compute_simple_checksum(corruptedData, 10240);
    hardware_CRC_checksum = hardware_CRC_calculation(corruptedData, 10240);
    printf("Simple Checksum: 0x%08X\n", simple_checksum);
    printf("Hardware CRC Checksum: 0x%08X\n", hardware_CRC_checksum);
    // Corrupting bit at position 0 of byte 8864 (2496 is has already been altered)
    corruptedData[8864] ^= 1;
    printf("After Corruption:\n");
    printf("Byte at 8864 = 0x%02X\n", corruptedData[8864]);
    simple_checksum = compute_simple_checksum(corruptedData, 10240);
    hardware_CRC_checksum = hardware_CRC_calculation(corruptedData, 10240);
    printf("Simple Checksum: 0x%08X\n", simple_checksum);
    printf("Hardware CRC Checksum: 0x%08X\n", hardware_CRC_checksum);

    while (1) {
    }
}
