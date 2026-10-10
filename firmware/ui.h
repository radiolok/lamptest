#ifndef UI_H
#define UI_H

// LCD screens
//
//   01234567890123456789
//   01 ECC83_J29 G-24.0V      slot, name, grid bias
//   H=12.6V A=300 G2=300      heater, anode and screen voltage
//   T 2550mA 199.9 19.99      error, Ih, Ia, Ig2
//   S=99.9 R=99.9 K=99.9      results (or T=countdown while measuring)

void ui_splash(void);
void ui_draw_labels(void);
void ui_draw(void);

#endif
