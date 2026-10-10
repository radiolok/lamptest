#include <string.h>
#include "app.h"
#include "config.h"
#include "convert.h"

volatile setpoints_t sp;
uint16_t ug1_ref;
uint16_t ug1_off;
volatile uint8_t err;
volatile uint8_t report_request;

uint8_t lamp_num;
lamp_t  lamp;
uint8_t field;
uint8_t lamp_fresh;

uint16_t live[PARAM_COUNT];

void app_init(void)
{
    memset((void *)&sp, 0, sizeof(sp));
    memset(&lamp, 0, sizeof(lamp));
    memset(live, 0, sizeof(live));
    err = 0;
    report_request = 0;
    field = F_LAMP;
    lamp_fresh = 0;
    lamp_num = LAMP_SUPPLY;

    ug1_off = ug1_ref = conv_ug1_to_adc(UG1_MAX);
    sp.ug1 = ug1_off;
    lamp.ug1def = UG1_MAX;
}
