set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm-none-eabi)

set(CMAKE_C_COMPILER	arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER	${CMAKE_C_COMPILER})
set(CMAKE_LINKER	${CMAKE_C_COMPILER})
set(CMAKE_OBJCOPY	$arm-none-eabi-objcopy)
set(CMAKE_SIZE		$arm-none-eabi-size)
# No gcc test using full environment library
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# MCU specific flags
set(TARGET_FLAGS "-mcpu=cortex-m3 ")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -MMD -MP")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mthumb -Wall -fdata-sections -ffunction-sections -fstack-usage")

set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "-O2 -g")

set(CMAKE_EXE_LINKER_FLAGS "${TARGET_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T \"${CMAKE_SOURCE_DIR}/STM32F103xx_FLASH.ld\"")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${CMAKE_PROJECT_NAME}.map -Wl,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--print-memory-usage")
