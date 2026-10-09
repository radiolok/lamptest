#ifndef CONFIG_H
#define CONFIG_H

// Hardware-independent constants of the tester. Everything here is plain
// numbers so the logic modules can be built and tested on the host.

#include <stdint.h>

//***** Clock and timing **************************************************
#define F_XTAL          16000000UL  // crystal frequency
#define UART_UBRR       103         // 9600 8N1 @ 16 MHz
#define TIMER2_1MS      250         // OCR2 value for a 1 ms tick (XTAL/64)
#define TICKS_PER_STEP  250         // 1 ms ticks per sequencer step (250 ms)

//***** Push button debounce (1 ms ticks) *********************************
#define BTN_DEBOUNCE    20          // released for this long = "released"
#define BTN_LONG        250         // pressed for this long = "held"

//***** Analog front end **************************************************
#define VREF            509u        // ADC reference, 10 mV units (5.09 V)

// ADC multiplexer channels (port A)
enum adc_channel {
    ADC_CH_TEMP = 0,    // LM35 heatsink sensor
    ADC_CH_IH   = 1,
    ADC_CH_UH   = 2,
    ADC_CH_UA   = 3,
    ADC_CH_IA   = 4,
    ADC_CH_UG2  = 5,
    ADC_CH_IG2  = 6,
    ADC_CH_UG1  = 7,
    ADC_CH_COUNT
};

#define ADC_BLOCK_SCANS     64      // scans averaged into one measurement

// Over-current trip levels (single 10-bit sample)
#define IH_TRIP_ADC         400
#define IA_TRIP_ADC         1020    // only on the 200 mA range
#define IG2_TRIP_ADC        1020
#define TRIP_SAMPLES        2       // consecutive extra samples before a trip

// Ia auto-range thresholds (single 10-bit sample)
#define IA_RANGE_UP_ADC     950     // 20 mA -> 200 mA
#define IA_RANGE_DOWN_ADC   85      // 200 mA -> 20 mA

// Heatsink temperature, sum of 64 samples: 80 C = 0.8 V / 5.12 V * 1024 * 64
#define TEMP_TRIP_SUM       10240   // 80 C
#define TEMP_CLEAR_SUM      8960    // 70 C

//***** Error flags *******************************************************
#define ERR_IH      0x01            // heater over-current
#define ERR_IA      0x02            // anode over-current
#define ERR_IG2     0x04            // screen over-current
#define ERR_TEMP    0x08            // heatsink over-temperature

//***** Measurement *******************************************************
#define UG1_MAX         240         // -24.0 V, also the "grid off" bias
#define UG1_DELTA_ADC   11          // grid step for S: +-11 ADC codes, about +-0.4 V
#define UA_DELTA        10          // +-10 V anode step for R
#define RESULT_MAX      999         // 99.9: S, R and K saturate here
#define WARMUP_TICKS_PER_MIN 240

#endif
