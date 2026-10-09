# Host unit tests of the firmware logic.
# Configure from the repository root with -DLAMPTEST_TESTS=ON.

set(FW ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(fw_logic STATIC
    ${FW}/adc_scan.c
    ${FW}/app.c
    ${FW}/button.c
    ${FW}/control.c
    ${FW}/convert.c
    ${FW}/editor.c
    ${FW}/format.c
    ${FW}/lamp.c
    ${FW}/lampdb.c
    ${FW}/panel.c
    ${FW}/sequencer.c
    fake_hal.c
    test_env.c
)
# fakes/ first: it provides <avr/pgmspace.h> and <avr/eeprom.h>
target_include_directories(fw_logic PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/fakes
    ${FW}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_options(fw_logic PUBLIC
    -std=gnu11
    -funsigned-char         # as on the AVR
    -Wall
    -Wextra
    -Wno-missing-braces
    -fsanitize=address,undefined
    -fno-sanitize-recover=undefined
    $<$<BOOL:${WERROR}>:-Werror>
)
target_link_options(fw_logic PUBLIC -fsanitize=address,undefined)

foreach(name format convert control adc_scan button sequencer editor panel lampdb)
    add_executable(test_${name} test_${name}.c)
    target_link_libraries(test_${name} PRIVATE fw_logic)
    add_test(NAME ${name} COMMAND test_${name})
endforeach()
