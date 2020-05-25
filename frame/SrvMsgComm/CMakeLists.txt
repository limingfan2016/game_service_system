cmake_minimum_required (VERSION 2.6)

SET(PROJECT_NAME SrvMsgComm)
project(${PROJECT_NAME})

#包含公共头文件
include(../../cmake/common.cmake)

#包含当前目录所有源文件
AUX_SOURCE_DIRECTORY(. ALL_SOURCES)

#生成二进制文件
add_executable(${PROJECT_NAME} ${ALL_SOURCES})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${MY_LIBRARY_LIST})