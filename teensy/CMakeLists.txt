
cmake_minimum_required (VERSION 3.16)

#cmake_policy(SET CMP0057 NEW)
#message("CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
set(PROJECT_NAME donkey)
PROJECT(${PROJECT_NAME})

option(LOAD_VIA_ROBOT "load the file to the teensy module via jetson nano" OFF)

set(PROJ_SRC_FILES  SL_${PROJECT_NAME})
set(CMAKE_COLOR_MAKEFILE TRUE)
set(CMAKE_VERBOSE_MAKEFILE ON)

SET(cdir "${CMAKE_SOURCE_DIR}")
set(xqt "${cdir}/../xqtools")
INCLUDE("${cdir}/cmake/options.cmake")
include("${xqt}/buildsys/ccmake.cmake")

ENABLE_LANGUAGE(ASM  C CXX)
set(SKIP_LIBS_LIST)

set(BUILD_DIR ${CMAKE_BINARY_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${BUILD_DIR}/debug")
set(ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${BUILD_DIR}/debug")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${BUILD_DIR}/debug")

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${BUILD_DIR}/release")
#set(ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${BUILD_DIR}/release")
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${BUILD_DIR}/release")

add_definitions(-DOURPROJECTNAME=${PROJECT_NAME}) 
#add_definitions(-DNOMINMAX)
add_definitions(-DUSE_SINGLE__PRECISION_NUMBERS -DXMPRINTF_TO_FILE)

add_definitions(-DLWIP_PROVIDE_ERRNO=1 -DLWIP_IPV4=1) # for QNEthernet library
#add_compile_options( -std=c++14 )
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 ")

set(ourGroupNames)
set (foldersList  
	src
	#nav
	pjrc/teensy4 
	pjrc/libraries
) 

#set(INC_DIR_LIST ${COREPATH}) # C:/programs/teensy/tools/arm/arm-none-eabi/include)

FOREACH(i ${foldersList})
	#message(STATUS ${i})
	addSourceFiles0(${i} ${i})
ENDFOREACH()

addSourceFiles(interface "../src" eth_api)

# add some of the matrix and 'common' files:
addSourceFiles(matrix "${xqt}/matrix/src" 
	xmatrix_imp xmatrix2 xmfilter xmgsl
)
	
addSourceFiles(core "${xqt}/core/src"
  rbuf xmroundbuf xmutils xstdef xmatrixplatform
)
 
addSourceFiles(core "${xqt}/message/src"
  xmessage xmessagesend
)

#message(STATUS "ourGroupNames = " ${ourGroupNames}) 

# process all the groups:
foreach(ourGroup  ${ourGroupNames})
	source_group(${ourGroup} FILES ${group_${ourGroup}_files})
	#message(" group ${ourGroup} created; ")
	#message(${group_${ourGroup}_files})
	#message("")
endforeach()

#message(STATUS "INC_DIR_LIST = ")
foreach(dir  ${INC_DIR_LIST})
	message(STATUS "  -I    ${dir}")
endforeach()

#message(STATUS "PROJ_SRC_FILES = ${${PROJ_SRC_FILES}}")

ADD_EXECUTABLE(${PROJECT_NAME} ${${PROJ_SRC_FILES}})
INCLUDE_DIRECTORIES(${INC_DIR_LIST}) 


SET_TARGET_PROPERTIES( ${PROJECT_NAME}
	PROPERTIES    
	DEBUG_OUTPUT_NAME "${PROJECT_NAME}d.elf"    
	RELEASE_OUTPUT_NAME "${PROJECT_NAME}.elf"
	OUTPUT_NAME "${PROJECT_NAME}.elf"

#	ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${BUILD_DIR}/debug"  
#	ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${BUILD_DIR}/release"
#	ARCHIVE_OUTPUT_DIRECTORY "${BUILD_DIR}/release"
)


if (WIN32)
	add_custom_command(TARGET ${PROJECT_NAME}  POST_BUILD 
		COMMAND echo  "-------teensy post buld started---------" 
		COMMAND ${CMAKE_SIZE} ARGS "${BUILD_DIR}/${PROJECT_NAME}.elf"
		COMMAND ${CMAKE_OBJCOPY} ARGS -O ihex -R .eeprom ${BUILD_DIR}/${PROJECT_NAME}.elf ${BUILD_DIR}/${PROJECT_NAME}.hex
		COMMAND  ${atools}/teensy_post_compile.exe 
			ARGS -file=${PROJECT_NAME} -path=${BUILD_DIR} -tools="${atools}"
		COMMAND echo  "teensy post buld completed" 
		BYPRODUCTS ${BUILD_DIR}/${PROJECT_NAME}.hex
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		COMMENT "---------teensy post build--------------" 
		VERBATIM 
		USES_TERMINAL
	)
else()
	add_custom_command(TARGET ${PROJECT_NAME}  POST_BUILD 
		COMMAND ${CMAKE_SIZE} ARGS "${BUILD_DIR}/${PROJECT_NAME}.elf"
		COMMAND ${CMAKE_OBJCOPY} ARGS -O ihex -R .eeprom ${BUILD_DIR}/${PROJECT_NAME}.elf ${BUILD_DIR}/${PROJECT_NAME}.hex
		#COMMAND  ${atools}/teensy_post_compile 
		#	ARGS -file=${PROJECT_NAME} -path=${BUILD_DIR} -tools="${atools}"
		#COMMAND echo  "teensy post buld completed" 
		BYPRODUCTS ${BUILD_DIR}/${PROJECT_NAME}.hex
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		COMMENT "making HEX file" 
		VERBATIM 
		USES_TERMINAL
	)

endif()

if (LOAD_VIA_ROBOT )
	add_custom_command(TARGET ${PROJECT_NAME}  POST_BUILD 
		COMMAND scp   "${BUILD_DIR}/${PROJECT_NAME}.hex" test@robot:/home/test/prog/donkey 
		COMMAND ssh   test@robot /home/test/prog/donkey/donkey_load
		COMMENT   "loading the code into the teensy board"
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		VERBATIM
		USES_TERMINAL
	)
else()
	add_custom_command(TARGET ${PROJECT_NAME}  POST_BUILD 
		COMMAND cp   "${BUILD_DIR}/${PROJECT_NAME}.hex" /home/test/prog/donkey 
		COMMAND /home/test/prog/donkey/donkey_load
		COMMENT   "loading the code into the teensy board"
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		VERBATIM
		USES_TERMINAL
	)
endif()




















