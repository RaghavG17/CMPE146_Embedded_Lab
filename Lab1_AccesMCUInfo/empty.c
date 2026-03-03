#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "ti_msp_dl_config.h"

#define FACTORY_BASE  0x41C40000U
#define FACTORY_WORDS 32U   // 0x00 .. 0x8c from your table

static const char *factory_acronyms[FACTORY_WORDS] = {
    "TRACEID",               // 0x00
    "DEVICEID",              // 0x04
    "USERID",                // 0x08
    "BSLPIN_UART",           // 0x0C
    "BSLPIN_I2C",            // 0x10
    "BSLPIN_INVOKE",         // 0x14
    "SRAMFLASH",             // 0x18
    "PLLSTARTUP0_4_8MHZ",    // 0x1C
    "PLLSTARTUP1_4_8MHZ",    // 0x20
    "PLLSTARTUP0_8_16MHZ",   // 0x24
    "PLLSTARTUP1_8_16MHZ",   // 0x28
    "PLLSTARTUP0_16_32MHZ",  // 0x2C
    "PLLSTARTUP1_16_32MHZ",  // 0x30
    "PLLSTARTUP0_32_48MHZ",  // 0x34
    "PLLSTARTUP1_32_48MHZ",  // 0x38
    "TEMP_SENSE0",           // 0x3C
    "RESERVED0",             // 0x40
    "RESERVED1",             // 0x44
    "RESERVED2",             // 0x48
    "RESERVED3",             // 0x4C
    "RESERVED4",             // 0x50
    "RESERVED5",             // 0x54
    "RESERVED6",             // 0x58
    "RESERVED7",             // 0x5c
    "RESERVED8",             // 0x60
    "RESERVED9",             // 0x64
    "RESERVED10",            // 0x68
    "RESERVED11",            // 0x7c
    "RESERVED12",            // 0x70
    "RESERVED13",            // 0x72
    "RESERVED14",            // 0x76
    "BOOTCRC"                // 0x8c
};

int main(void)
{
    SYSCFG_DL_init();

    printf("Reading FACTORYREGION_TYPEA at 0x%08X\n\n", (unsigned)FACTORY_BASE);

    for (uint32_t i = 0; i < FACTORY_WORDS; ++i) {
        uint32_t addr = FACTORY_BASE + (i * 4U);
        uint32_t val  = *(volatile uint32_t *)addr;
        printf("%-18s 0x%08X  0x%08X\n",
               factory_acronyms[i], (unsigned)addr, (unsigned)val);
    }

    // USERID is *defined* by the table to be at offset 0x08
    uint32_t userid_addr  = FACTORY_BASE + 0x08U;
    uint32_t userid_value = *(volatile uint32_t *)userid_addr;

    printf("\nUSERID (from table) @ 0x%08X = 0x%08X\n",
           (unsigned)userid_addr, (unsigned)userid_value);

    while (1) { }
}
