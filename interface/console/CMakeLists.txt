#
# 	ethernet client console
#

cmake_minimum_required (VERSION 3.21)
set(xqt "${CMAKE_CURRENT_SOURCE_DIR}/../../xqtools")
include("${xqt}/buildsys/ccmake.cmake")
set(PROJECT_NAME console)

set (NEED_BOOST  program_options date_time system filesystem thread chrono log iostreams)
commonStart()
set(NO_VERSION_INFO 1)

add_compile_definitions(tstype=int)

#add_definitions(-DNOMINMAX)
#add_definitions(-D_WIN32_WINNT=0x0A00)
#add_definitions(-DUNICODE -D_UNICODE)

#IF(WIN32)

#endif()

addSourceFiles("main" "." console inproc eth_client)
addSourceFiles("core" "${xqt}/core/src" rbuf xmroundbuf xmutils xstdef xmatrixplatform)
addSourceFiles("matrix" "${xqt}/matrix/src" xmatrix_imp xmatrix2)
addSourceFiles("message" "${xqt}/message/src" xmessage xparser xq_parser datafileloader xqdatatxtfile xmessagepack xmessagesend)


#addOtherLib("Winmm.lib")
if (WIN32)
	addOtherLib("ws2_32.lib")
else()
	addOtherLib(ncurses)
endif()

programEnd()


