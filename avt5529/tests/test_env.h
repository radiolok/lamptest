#ifndef TEST_ENV_H
#define TEST_ENV_H

#include <stdint.h>
#include "config.h"

// Simulates the free-running ADC: each sample returns the value of the
// channel that was selected when its conversion started.
void sim_adc_samples(const uint16_t values[ADC_CH_COUNT], unsigned n);
void sim_adc_blocks(const uint16_t values[ADC_CH_COUNT], unsigned blocks);
#define SAMPLES_PER_SCAN    14
#define SAMPLES_PER_BLOCK   (SAMPLES_PER_SCAN * ADC_BLOCK_SCANS)

// Drive the push button for n ms
void button_for(uint8_t pressed, unsigned ms);
void button_hold(void);         // long press, still held
void button_release(void);      // released and debounced

// Select a slot as the main loop would
void select_lamp(uint8_t num);

// Sequencer steps until `start` reaches `point` (at most `limit` steps)
void seq_run_until(uint16_t point, unsigned limit);

#endif
