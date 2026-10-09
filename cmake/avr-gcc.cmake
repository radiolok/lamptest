# CMake toolchain file for the avr-gcc cross compiler.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)

find_program(AVR_CC avr-gcc REQUIRED)
find_program(AVR_OBJCOPY avr-objcopy REQUIRED)
find_program(AVR_SIZE avr-size REQUIRED)

set(CMAKE_C_COMPILER ${AVR_CC})
set(CMAKE_ASM_COMPILER ${AVR_CC})
set(CMAKE_OBJCOPY ${AVR_OBJCOPY} CACHE FILEPATH "")

# Cross compiler cannot link a host test program.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
