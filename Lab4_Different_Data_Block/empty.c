#include "ti_msp_dl_config.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define CRC32_SEED     0xFFFFFFFF
#define TIMER_FREQ_HZ  3200000

#define MAX_DATA_SIZE  10240

/* Use the same data block, myData */
static uint8_t myData[MAX_DATA_SIZE];

/* Global size array exactly as the lab states */
int size_array[] = {2, 4, 16, 32, 64, 128, 256, 786, 1024, 2048, 4096, 8192, 10240};

/* ===== DMA setup (from the lab handout) ===== */
#define DMA_CH0_CHAN_ID (0)

static const DL_DMA_Config gDMA_CH0Config = {
    .transferMode  = DL_DMA_SINGLE_BLOCK_TRANSFER_MODE,
    .extendedMode  = DL_DMA_NORMAL_MODE,
    .destIncrement = DL_DMA_ADDR_UNCHANGED,
    .srcIncrement  = DL_DMA_ADDR_INCREMENT,
    .destWidth     = DL_DMA_WIDTH_BYTE,
    .srcWidth      = DL_DMA_WIDTH_BYTE,
    .trigger       = DMA_SOFTWARE_TRIG,
    .triggerType   = DL_DMA_TRIGGER_TYPE_EXTERNAL,
};

volatile bool dma_done;

void DMA_IRQHandler(void)
{
    switch (DL_DMA_getPendingInterrupt(DMA)) {
        case DL_DMA_EVENT_IIDX_DMACH0:
            dma_done = true;
            break;
        default:
            break;
    }
}
/* =========================================== */

int main(void)
{
    SYSCFG_DL_init();

    /* Fill myData (same idea as your Lab 3 Ex 2.2) */
    srand(12345);
    for (int i = 0; i < MAX_DATA_SIZE; i++) {
        myData[i] = rand();
    }

    /* CRC setup */
    DL_CRC_reset(CRC);
    DL_CRC_enablePower(CRC);
    delay_cycles(POWER_STARTUP_DELAY);
    DL_CRC_init(CRC,
        DL_CRC_32_POLYNOMIAL,
        DL_CRC_BIT_REVERSED,
        DL_CRC_INPUT_ENDIANESS_LITTLE_ENDIAN,
        DL_CRC_OUTPUT_BYTESWAP_DISABLED);

    /* Timer setup to use the 32-bit counter TIMG12 */
    DL_Timer_enablePower(TIMG12);

    DL_Timer_ClockConfig config;
    config.clockSel     = DL_TIMER_CLOCK_BUSCLK;
    config.divideRatio  = DL_TIMER_CLOCK_DIVIDE_1;
    config.prescale     = 0;
    DL_Timer_setClockConfig(TIMG12, &config);

    DL_Timer_TimerConfig timerConfig;
    timerConfig.timerMode     = DL_TIMER_TIMER_MODE_PERIODIC_UP;
    timerConfig.period        = (uint32_t)-1;
    timerConfig.startTimer    = DL_TIMER_START;
    timerConfig.genIntermInt  = DL_TIMER_INTERM_INT_DISABLED;
    timerConfig.counterVal    = 0;
    DL_Timer_initTimerMode(TIMG12, &timerConfig);

    /* DMA setup code — called only once (not inside the loop) */
    DL_DMA_clearInterruptStatus(DMA, DL_DMA_INTERRUPT_CHANNEL0);
    DL_DMA_enableInterrupt(DMA, DL_DMA_INTERRUPT_CHANNEL0);
    DL_DMA_initChannel(DMA, DMA_CH0_CHAN_ID, (DL_DMA_Config *)&gDMA_CH0Config);
    NVIC_EnableIRQ(DMA_INT_IRQn);

    /* Loop over each block size and repeat Ex 1.1 measurements */
    int nSizes = (int)(sizeof(size_array) / sizeof(size_array[0]));

    for (int s = 0; s < nSizes; s++) {

        uint32_t DATA_SIZE = (uint32_t)size_array[s];

        uint32_t start_count, end_count, diff_cycles;
        uint32_t hw_time_us, dma_time_us;
        uint32_t hw_crc, dma_crc;

        printf("\nBlock size: %lu bytes\n", (unsigned long)DATA_SIZE);

        /* ===== Hardware method (CPU feeds CRC accelerator) ===== */
        DL_CRC_setSeed32(CRC, CRC32_SEED);

        start_count = DL_Timer_getTimerCount(TIMG12);
        for (uint32_t i = 0; i < DATA_SIZE; i++) {
            DL_CRC_feedData8(CRC, myData[i]);
        }
        end_count = DL_Timer_getTimerCount(TIMG12);

        hw_crc = DL_CRC_getResult32(CRC);

        diff_cycles = end_count - start_count;
        hw_time_us = (uint32_t)(((uint64_t)diff_cycles * 1000000) / TIMER_FREQ_HZ);

        /* ===== DMA method (DMA transfers bytes to CRCIN) ===== */
        DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&myData[0]);
        DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, DL_CRC_getCRCINAddr(CRC));
        DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, DATA_SIZE);
        DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);

        DL_CRC_setSeed32(CRC, CRC32_SEED);

        dma_done = false;

        start_count = DL_Timer_getTimerCount(TIMG12);
        DL_DMA_startTransfer(DMA, DMA_CH0_CHAN_ID);
        while (!dma_done) {
            /* wait for ISR to set dma_done */
        }
        end_count = DL_Timer_getTimerCount(TIMG12);

        dma_crc = DL_CRC_getResult32(CRC);

        diff_cycles = end_count - start_count;
        dma_time_us = (uint32_t)(((uint64_t)diff_cycles * 1000000) / TIMER_FREQ_HZ);

        /* Output: checksums must match, show times and speedup */
        printf("Hardware checksum: 0x%08X\n", hw_crc);
        printf("DMA checksum:      0x%08X\n", dma_crc);

        printf("Hardware time: %u us\n", hw_time_us);
        printf("DMA time:      %u us\n", dma_time_us);

        if (dma_time_us != 0) {
            float speedup = (float)hw_time_us / (float)dma_time_us;
            printf("Speedup (Hardware time / DMA time): %.2f\n", speedup);
        } else {
            printf("Speedup (Hardware time / DMA time): N/A\n");
        }
    }

    while (1) {
    }
}