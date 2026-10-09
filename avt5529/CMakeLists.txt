# AVT5229 vacuum tube tester firmware, ATmega32 @ 16 MHz
set(MCU   atmega32)
set(F_CPU 16000000UL)

add_executable(avt5229
    avt5229.c
    lcd.c
    util.c
)
set_target_properties(avt5229 PROPERTIES SUFFIX ".elf")

# Same options as the Atmel Studio project (avt5529.cproj)
target_compile_options(avt5229 PRIVATE
    -mmcu=${MCU}
    -std=gnu11
    -funsigned-char
    -funsigned-bitfields
    -fpack-struct
    -fshort-enums
    -ffunction-sections
    -fdata-sections
    -fno-common
    -Wall
    -Wextra
    -Wno-missing-braces   # lamprom.h initialises name[9] without inner braces
    $<$<BOOL:${WERROR}>:-Werror>
)
target_compile_definitions(avt5229 PRIVATE F_CPU=${F_CPU})
target_link_options(avt5229 PRIVATE
    -mmcu=${MCU}
    -Wl,--gc-sections
    -Wl,-Map=$<TARGET_FILE_DIR:avt5229>/avt5229.map
)

# Flash (.hex) and EEPROM (.eep) images, plus memory usage report
add_custom_command(TARGET avt5229 POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex -R .eeprom -R .fuse -R .lock -R .signature
            $<TARGET_FILE:avt5229> avt5229.hex
    COMMAND ${CMAKE_OBJCOPY} -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load
            --change-section-lma .eeprom=0 --no-change-warnings
            $<TARGET_FILE:avt5229> avt5229.eep
    COMMAND ${AVR_SIZE} -C --mcu=${MCU} $<TARGET_FILE:avt5229>
    WORKING_DIRECTORY $<TARGET_FILE_DIR:avt5229>
    BYPRODUCTS avt5229.hex avt5229.eep avt5229.map
    VERBATIM
)
