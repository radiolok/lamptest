#ifndef PANEL_H
#define PANEL_H

#include <stdint.h>

// Main loop work: loads the selected record, converts the ADC averages
// into live readings, saves edited fields and renders everything into the
// report line, which is both the LCD content and the serial output.

// Report line layout: fields separated by '\0'
enum report_pos {
    RPT_NUM  = 0,   // "NN"
    RPT_NAME = 3,   // 9 characters
    RPT_UH   = 13,  // "12.6"
    RPT_IH   = 18,  // "1250" mA
    RPT_UG1  = 23,  // "24.0"
    RPT_UA   = 28,  // "300"
    RPT_IA   = 32,  // "19.99" or "199.9"
    RPT_UG2  = 38,  // "300"
    RPT_IG2  = 42,  // "39.99"
    RPT_S    = 48,  // "99.9"
    RPT_R    = 53,
    RPT_K    = 58,
    RPT_LEN  = 62
};

void panel_init(void);
void panel_update(void);

const char *panel_report(void);

#endif
