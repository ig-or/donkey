

#  based on https://github.com/apmorton/teensy-template

INCLUDE(CMakeForceCompiler)
message(STATUS ${CMAKE_GENERATOR})
#string(COMPARE EQUAL ${CMAKE_GENERATOR} "Ninja" tbProto)
#if (tbProto)#
#	message(STATUS "creating a TinkerBell project")
#else()
#	message(STATUS "creating a VS project")
#endif()

set(CMAKE_SYSTEM_NAME Generic)
#set (CMAKE_SYSTEM_VERSION )
set(CMAKE_SYSTEM_PROCESSOR arm)
include("${CMAKE_CURRENT_LIST_DIR}/options.cmake")

set(gcc_path "$ENV{GCC_ARM}")
cmake_path(NORMAL_PATH gcc_path)  # need this for Windows

message(STATUS "atools =  ${atools}; gcc path = ${gcc_path}")
#SET(ASM_OPTIONS "-x assembler-with-cpp")

SET(CMAKE_FIND_ROOT_PATH  ${TOOLSPATH}/arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE   STATIC_LIBRARY)

set(COMPILERPATH  "${gcc_path}/bin")

set(esfx)
if (WIN32)
set(esfx .exe)
endif()

set(CMAKE_AR  ${COMPILERPATH}/arm-none-eabi-ar${esfx})
set(CMAKE_C_COMPILER "${COMPILERPATH}/arm-none-eabi-gcc${esfx}")
set(CMAKE_CXX_COMPILER "${COMPILERPATH}/arm-none-eabi-g++${esfx}")
set(CMAKE_ASM_COMPILER "${COMPILERPATH}/arm-none-eabi-gcc${esfx}")

#set(CMAKE_ASM_COMPILER "\"${COMPILERPATH}/arm-none-eabi-as${esfx}")
#set(CMAKE_ASM_NASM_COMPILER "\"${COMPILERPATH}/arm-none-eabi-as${esfx}")
#set(CMAKE_ASM_MASM_COMPILER "\"${COMPILERPATH}/arm-none-eabi-as${esfx}")
#set(CMAKE_ASM-ATT_COMPILER "\"${COMPILERPATH}/arm-none-eabi-as${esfx}")
set(CMAKE_LINKER  "${COMPILERPATH}/arm-none-eabi-ld${esfx}")

set(CMAKE_OBJCOPY  "${COMPILERPATH}/arm-none-eabi-objcopy${esfx}"  CACHE INTERNAL "")
set(CMAKE_OBJDUMP "${COMPILERPATH}/arm-none-eabi-objdump${esfx}"  CACHE INTERNAL "")
set(CMAKE_SIZE  "${COMPILERPATH}/arm-none-eabi-size${esfx}"  CACHE INTERNAL "")
set(CMAKE_RANLIB "${COMPILERPATH}/arm-none-eabi-gcc-ranlib${esfx}"  CACHE INTERNAL "")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)



# options needed by many Arduino libraries to configure for Teensy model
#OPTIONS += -D__$(MCU)__ -DARDUINO=10813 -DTEENSYDUINO=154 -D$(MCU_DEF)
set(CMAKE_VERBOSE_MAKEFILE ON  CACHE INTERNAL "")

#set (BUILDDIR  build)
#set (BUILD_DIR  build)

# compiler options for C only
#set (CFLAGS )
set(CMAKE_C_FLAGS "${CPPFLAGS}"  CACHE INTERNAL "" FORCE)
set(CMAKE_CXX_FLAGS "${CPPFLAGS} ${CXXFLAGS}"  CACHE INTERNAL "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS ${LDFLAGS}  CACHE INTERNAL "" FORCE)

#add_compile_options(${COREPATH}/bootdata.c)

message (STATUS "COREPATH = ${COREPATH}")
message (STATUS "LDSCRIPT = ${LDSCRIPT}")
message (STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
message (STATUS "CMAKE_C_FLAGS = ${CMAKE_C_FLAGS}")
message (STATUS "CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
message (STATUS "CMAKE_EXE_LINKER_FLAGS = ${CMAKE_EXE_LINKER_FLAGS}")

#ASMFLAGS, ASM_NASMFLAGS, ASM_MASMFLAGS or ASM-ATTFLAGS
set (asmFlags  " ${CPPFLAGS} -x assembler-with-cpp")

set(ASMFLAGS  "${asmFlags}"   CACHE INTERNAL "" FORCE)
set(ASM_NASMFLAGS  "${asmFlags}"  CACHE INTERNAL "" FORCE )
set(ASM_MASMFLAGS  "${asmFlags}"   CACHE INTERNAL "" FORCE)
set(ASM_ATTFLAGS  "${asmFlags}"   CACHE INTERNAL "" FORCE)

set(CMAKE_ASM_FLAGS  "${asmFlags}"  CACHE INTERNAL "" FORCE )
set(CMAKE_NASM_FLAGS  "${asmFlags}"  CACHE INTERNAL "" FORCE)
set(CMAKE_MASM_FLAGS  "${asmFlags}"  CACHE INTERNAL "" FORCE)
set(CMAKE_ATT_FLAGS  "${asmFlags} " CACHE INTERNAL "" FORCE )

set(BUILD_SHARED_LIBS OFF)

