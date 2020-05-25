cmake_minimum_required (VERSION 2.6)

SET(PROJECT_NAME httpbase)
project(${PROJECT_NAME})

#包含公共头文件
include(../../cmake/common.cmake)

#包含当前目录所有源文件
AUX_SOURCE_DIRECTORY(protocol PROTO_SOURCES)
AUX_SOURCE_DIRECTORY(. CUR_SOURCES)
SET(ALL_SOURCES ${CUR_SOURCES} ${PROTO_SOURCES})

#Generate the static library from the library sources
add_library(${PROJECT_NAME} STATIC ${ALL_SOURCES})