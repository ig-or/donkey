
file(REAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../pjrc/teensy4" COREPATH)
set(atools "$ENV{TEENSY_TOOLS}")

# Use these lines for Teensy 4.1
set(MCU IMXRT1062)
set(MCU_LD imxrt1062_t41.ld)
set(MCU_DEF ARDUINO_TEENSY41)
set(TEENSY_CORE_SPEED 600000000)
set(AARDUINO 10607)

set(CDEFS "-DUSB_SERIAL -DLAYOUT_US_ENGLISH -DUSING_MAKEFILE -DXM_ARM -D__arm__ -DF_CPU=${TEENSY_CORE_SPEED} -D__${MCU}__  -DARDUINO=${AARDUINO} -DTEENSYDUINO=157 -D${MCU_DEF} ")
# for Cortex M7 with single & double precision FPU
set(CPUOPTIONS " -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16 -mthumb ")  #-ffast-math
# use this for a smaller, no-float printf
#SPECS = --specs=nano.specs

# CPPFLAGS = compiler options for C and C++
#set(CPPFLAGS " -Wall -g -O2 -mthumb -ffunction-sections -fdata-sections -MMD ")   #    -nostdlib
set(CPPFLAGS " -Wall -O2 ${CPUOPTIONS} ${CDEFS} -MMD -I. -ffunction-sections -fdata-sections    ")  #  -fstack-usage # -Wno-switch " ) #  -Wdouble-promotion " -Wno-switch -funwind-tables  -fasynchronous-unwind-tables )

# compiler options for C++ only
set(CXXFLAGS  " -std=gnu++14 -felide-constructors -fno-exceptions -fpermissive -fno-rtti -Wno-error=narrowing  -fno-threadsafe-statics ") #-fsized-deallocation


# additional libraries to link
set(LIBS   "-larm_cortexM7lfsp_math -lm -lstdc++" )

# linker options
#set (LDFLAGS " -Os -Wl,--gc-sections -mthumb ") # -LC:/programs/teensy/tools/arm/arm-none-eabi/lib")
set(LDSCRIPT  "\"${COREPATH}/${MCU_LD}\"")
#set (LDFLAGS " -O2 -Wl,--gc-sections,--relax ${SPECS} ${CPUOPTIONS} -T${LDSCRIPT} ")
set (LDFLAGS "-Os -Wl,--gc-sections,--relax -T${LDSCRIPT} ${LIBS} ") # -fPIC
#set (LDFLAGS "${LDFLAGS} \"-L${gcc_path}/lib/gcc/arm-none-eabi/10.3.1/thumb/v7e-m+fp/hard\"")
#set (LDFLAGS "${LDFLAGS} \"-L${gcc_path}/arm-none-eabi/lib/thumb/v7e-m+fp/hard\"")
set (LDFLAGS "${LDFLAGS} \"-L${atools}/../../teensy-compile/11.3.1/arm/arm-none-eabi/lib\"") 
#set (LDFLAGS "${LDFLAGS} \"-L${atools}/../../teensy-compile/11.3.1/arm/arm-none-eabi/lib/armv7e-m/fpu/fpv5-d16\"")

#set(LDFLAGS "${LDFLAGS}   -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16 -T${LDSCRIPT} " ) # -u _scanf_float  -u _printf_float")
#if (tbProto)
#	set(LDFLAGS "${LDFLAGS}  -u _scanf_float  -u _printf_float ")
#endif()

