# lamptest

[![Build firmware](https://github.com/radiolok/lamptest/actions/workflows/build.yml/badge.svg)](https://github.com/radiolok/lamptest/actions/workflows/build.yml)

A modern MCU-based tester for vacuum tubes.

This repository is based on the Polish **AVT5229** vacuum tube tester
("Miernik lamp elektronowych", firmware "VTTester 1.16"). It contains:

* **`avt5529/`**: the AVR firmware, ported from the original ICCAVR sources to avr-gcc
* **`avt/`**: a KiCad 9 redraw of the schematic and PCB, with 3D models
* **`avt5529/AVT5229.pdf`**: the original magazine article (in Polish)

![PCB, 3D view](docs/images/pcb_iso.png)

| Top view | Schematic |
|---|---|
| ![PCB top](docs/images/pcb_top.png) | [![Schematic](docs/images/schematic.svg)](docs/images/schematic.svg) |

## What it does

The tester applies the operating voltages from a tube's datasheet. It then
measures the tube's static parameters and its small-signal parameters:

| Quantity | Range | Resolution |
|---|---|---|
| Heater voltage U<sub>h</sub> (or heater current I<sub>h</sub> for series-heated P/U tubes) | 0–15.0 V / 0–2.50 A | 0.1 V / 10 mA |
| Grid bias −U<sub>g1</sub> | 0 … −24.0 V | 0.1 V |
| Anode voltage U<sub>a</sub> | 0–300 V | 1 V |
| Anode current I<sub>a</sub> (auto-range 20 / 200 mA) | 0–200.0 mA | 0.01 / 0.1 mA |
| Screen voltage U<sub>g2</sub> | 0–300 V | 1 V |
| Screen current I<sub>g2</sub> | 0–40.00 mA | 0.01 mA |
| Transconductance S = ΔI<sub>a</sub>/ΔU<sub>g1</sub> | 0–99.9 mA/V | 0.1 |
| Internal resistance R = ΔU<sub>a</sub>/ΔI<sub>a</sub> | 0–99.9 kΩ | 0.1 |
| Amplification factor K = S·R | 0–99.9 | 0.1 |

### Measurement sequence

The sequence runs in 250 ms ticks, driven by `TIMER2_COMP_vect`:

1. Switch on the heater, then wait out the warm-up time (1–9 min, set per tube).
2. Set U<sub>g1</sub> = U<sub>g</sub> − 1.1 V, then ramp up U<sub>a</sub> and U<sub>g2</sub>. Read I<sub>a</sub>.
3. Set U<sub>g1</sub> = U<sub>g</sub> + 1.1 V and read I<sub>a</sub> again. This gives **S**.
4. Return U<sub>g1</sub> to U<sub>g</sub>. Read I<sub>a</sub> at U<sub>a</sub> − 10 V and at U<sub>a</sub> + 10 V. This gives **R**, and **K** = S·R.
5. Latch the results on the LCD and send them over RS-232.
6. Ramp down U<sub>g2</sub>, U<sub>a</sub> and U<sub>g1</sub>, beep, and switch off the heater.

Press the knob to start a measurement. While the results are on screen, press it
again to re-measure without waiting for warm-up. Turn the knob to abort a
measurement or leave the results screen, which switches off the heater.

### Protections

* **Heater overcurrent**: the heater is switched off (`H` on the LCD).
* **Anode overcurrent** on the 200 mA range: U<sub>a</sub> and U<sub>g2</sub> are switched off (`A`).
* **Screen overcurrent**: U<sub>g2</sub> is switched off (`G`).
* **Heatsink over-temperature** (LM35 sensor): trips above 80 °C, clears below 70 °C (`T`).
* Any error aborts the measurement and sounds the buzzer.
* The watchdog timeout is about 1 s.

## Hardware

* **MCU**: ATmega32 with a 16 MHz crystal. The schematic symbol is the pin-compatible ATmega16A-P.
* **Display**: 4×20 HD44780 LCD in 4-bit mode.
* **Input**: rotary encoder with a push button, and a buzzer.
* **Heater supply**: PWM-regulated (OC0). It runs in voltage mode, or in current mode for series-heater tubes.
* **U<sub>a</sub> and U<sub>g2</sub>**: two 300 V PWM step-up converters (OC1B, OC1A) with IRF740 switches and TC4426 drivers.
* **U<sub>g1</sub>**: negative bias from an MCU-clocked charge pump (`CLKUG1`). It is regulated sample-by-sample in the ADC ISR.
* **Relays**: one relay switches between anode sections (UA1/UA2). Another switches the I<sub>a</sub> shunt (20/200 mA).
* **Interface**: RS-232 through a MAX202, 9600 8N1.

### MCU pinout

| Port | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| **A** (ADC) | U<sub>g1</sub> | I<sub>g2</sub> | U<sub>g2</sub> | I<sub>a</sub> | U<sub>a</sub> | U<sub>h</sub> | I<sub>h</sub> | Temp (LM35) |
| **B** | SCK | MISO | MOSI | K1 | U<sub>h</sub> PWM (OC0) | U<sub>g1</sub> pump clock | Anode select | I<sub>a</sub> range |
| **C** | LCD D7 | LCD D6 | LCD D5 | LCD D4 | LCD E | LCD RS | SDA | SCL |
| **D** | Buzzer (OC2) | Encoder DIR | U<sub>g2</sub> PWM (OC1A) | U<sub>a</sub> PWM (OC1B) | Encoder CLK (INT1) | Encoder button | TXD | RXD |

## Tube database

**Flash (81 entries, read-only).** Slot `00` is a **bench power supply** mode,
where every voltage can be set by hand. Slot `01` is reserved. Slots `02`–`80`
are preset tubes, with each triode section of a twin triode stored as its own
entry:

ECC81, ECC82, ECC83, ECC85, ECC88, ECC91, ECC99, ECC35, ECC40, ECC801, ECC802, ECC803, ECC832,
E180CC, E182CC, PCC84, PCC85, PCC88, 6N1P, 6N2P, 6N3P, 6N6P, 6N30P, 6N5S, 6N7S, 6SL7, 6SN7, 6SC7,
6AS7, 6S19P, 6S2S, EF86, 6SJ7, EL34, EL84, EL90, EL95, 6V6S, 6F6S, 6L6G, 6P1P, KT66, KT77, KT88,
ECL82, ECL86, PCL86.

**EEPROM (19 entries, slots 81–99, user-editable).** You can edit every field
with the encoder: name, socket, electrode system, warm-up time, every voltage
and current, and the reference values for S, R and K.

Each name has 9 characters: `TTTTTT` `s` `e` `m`.

* `TTTTTT` is the tube type.
* `s` is the socket/adapter letter, A–J.
* `e` is the electrode system: `0` for a single tube, `1` or `2` for a section of a twin tube.
* `m` is the heater warm-up time in minutes.

For example, `ECC83_G11` means ECC83, socket G, section 1, 1 minute warm-up.

## Serial output

Press `ESC` in a terminal at 9600 8N1 to get a copy of the LCD. The tester
also sends a line automatically after every measurement:

```
Nr Type Uh[V] Ih[mA] -Ug[V] Ua[V] Ia[mA] Ug2[V] Ig2[mA] S[mA/V] R[k] K[V/V]
```

## Building the firmware

You need CMake ≥ 3.16 and the avr-gcc toolchain. On Debian or Ubuntu, install
`gcc-avr`, `binutils-avr` and `avr-libc`.

```sh
cmake -B build            # the AVR toolchain file is picked automatically
cmake --build build
```

The build writes these files to `build/avt5529/`:

* `avt5229.hex`: the flash image
* `avt5229.eep`: the EEPROM image, with the user tube table
* `avt5229.elf` and `avt5229.map`

It also prints the memory usage. To treat warnings as errors, add `-DWERROR=ON`.

The Atmel Studio 7 project (`avt5529/avt5529.cproj`) is still there for Windows users.

GitHub Actions builds every push and pull request
([.github/workflows/build.yml](.github/workflows/build.yml)). The `.hex`, `.eep`,
`.elf` and `.map` files are uploaded as build artifacts.

### Flashing

The fuse values come from `definitions.h`:

* **Low fuse 0xEF**: external crystal, BOD off.
* **High fuse 0xC9**: JTAG off, which is required because PORTC drives the LCD. SPI programming stays on, and CKOPT is set.

```sh
avrdude -c usbasp -p m32 -U lfuse:w:0xEF:m -U hfuse:w:0xC9:m
avrdude -c usbasp -p m32 -U flash:w:build/avt5529/avt5229.hex:i \
                         -U eeprom:w:build/avt5529/avt5229.eep:i
```

> **Note:** writing `.eep` replaces your user-defined tubes (slots 81–99) with
> the defaults.

## Firmware fixes in this port

These bugs were found and fixed while bringing the avr-gcc port up:

| Problem | Effect | Fix |
|---|---|---|
| `tick_1ms` and `uart_busy`, which are set from ISRs, were not `volatile` | `-Os` turned `delay()` and `char2rs()` into endless loops. The firmware hung after start-up and the watchdog reset it about once a second. The compiler had even dropped the rest of `main()` as unreachable. | Make the ISR-shared flags `volatile` |
| `eeprom_read_block()` had its arguments swapped, copied from the ICCAVR call order | Selecting a user tube (slot ≥ 81) copied EEPROM data over the CPU registers and I/O space | Fix the argument order |
| `eeprom_write_word()` was used on 1-byte fields (name, U<sub>g1</sub>, U<sub>h</sub>, I<sub>h</sub>) | Editing one field zeroed the next one. For example, saving U<sub>g1</sub> cleared the low byte of U<sub>a</sub>, and saving the warm-up time cleared U<sub>h</sub>. | Use `eeprom_write_byte()` |
| `poptyp` (the last selected tube) was a RAM variable, but it was accessed with the EEPROM API | Every measurement wrote to the EEPROM address equal to its RAM address, which corrupted user tube slot 86 | Store it in the EEPROM byte right after the user table, which keeps the table layout. Add a range check and use `eeprom_update_byte()`. |
| The refactored `fp2ascii()` never advanced past the `'.'`, and it blanked *every* `0` in the integer part | Displayed values were wrong (`12.6` → `126`, `100` → `1  `) and the LCD fields were misaligned | Rewrite it to match the original formatting, and check it on the host |
| `temp_str[]` was defined in a header; prototypes were missing | Link error with `-fno-common` (GCC ≥ 10) and implicit-declaration warnings | Move it into `fp2ascii()` and add the prototypes |

### Known limitations (not changed)

* Some 16-bit variables shared with ISRs, such as `start`, are read in the main loop without blocking interrupts.
* The S calculation only guards against ΔI<sub>a</sub> = 0, not against ΔU<sub>g</sub> = 0.

## Links

* AVT5229 kit (PCB + programmed MCU): <https://sklep.avt.pl/miernik-lamp-elektronowych-plytka-drukowana-i-zaprogramowany-uklad.html>

## License

MIT. See [LICENSE](LICENSE). The original AVT5229 design and firmware belong to their authors and AVT.
