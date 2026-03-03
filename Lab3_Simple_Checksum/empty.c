#include "ti_msp_dl_config.h"
#include <stdio.h>
#include <stdlib.h>

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

    // Printing the first four bytes
    printf("First four bytes: 0x%02X, 0x%02X, 0x%02X, 0x%02X\n", myData[0], myData[1], myData[2], myData[3]);
    // Printing the last four bytes
    printf("Last four bytes: 0x%02X, 0x%02X, 0x%02X, 0x%02X\n", myData[10236], myData[10237], myData[10238], myData[10239]);
    // Computing checksum
    uint32_t checksum_value = compute_simple_checksum(myData, 10240);
    printf("Checksum: 0x%08X\n", checksum_value);

    while (1) {
    }
}
