#
# 	jetson nano control alg
#

cmake_minimum_required (VERSION 3.21)
set(xqt "${CMAKE_CURRENT_SOURCE_DIR}/../../xqtools")
include("${xqt}/buildsys/ccmake.cmake")
set(PROJECT_NAME eye)

set (NEED_BOOST  program_options date_time system filesystem thread chrono log iostreams)
commonStart()
set(NO_VERSION_INFO 1)

add_compile_definitions(tstype=int)

find_package(ydlidar_sdk REQUIRED)
include_directories(
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/src
  ${YDLIDAR_SDK_INCLUDE_DIRS})
link_directories(${YDLIDAR_SDK_LIBRARY_DIRS})

#add_definitions(-DNOMINMAX)
#add_definitions(-D_WIN32_WINNT=0x0A00)
#add_definitions(-DUNICODE -D_UNICODE)

#IF(WIN32)

#endif()

addSourceFiles("main" "." main  eth_client g4)
addSourceFiles("core" "${xqt}/core/src" rbuf xmroundbuf xmutils xstdef xmatrixplatform)
addSourceFiles("matrix" "${xqt}/matrix/src" xmatrix_imp xmatrix2)
addSourceFiles("message" "${xqt}/message/src" xmessage xparser xq_parser datafileloader xqdatatxtfile xmessagepack xmessagesend)


#addOtherLib("Winmm.lib")

addOtherLib(${YDLIDAR_SDK_LIBRARIES})


programEnd()


