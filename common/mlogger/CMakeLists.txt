set (PROJECT mlogger)
project (${PROJECT})

include_directories(./include)
include_directories(./src)

set (HEADERS
    include/mlogger.h
	include/mlogger_only.h)

set (SOURCES
    src/mlogger.cpp)

add_library(${PROJECT} ${HEADERS} ${SOURCES})