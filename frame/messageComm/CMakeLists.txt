cmake_minimum_required (VERSION 2.6)

SET(PROJECT_NAME messageComm)
project(${PROJECT_NAME})

#包含公共头文件
include(../../cmake/common.cmake)

#包含当前目录所有源文件
AUX_SOURCE_DIRECTORY(. ALL_SOURCES)

#Generate the static library from the library sources
add_library(${PROJECT_NAME} STATIC ${ALL_SOURCES})