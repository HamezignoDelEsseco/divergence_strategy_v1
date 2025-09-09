cmake_minimum_required(VERSION 4.0)

include_directories("D:\\SierraChartCME\\ACS_Source" "{CMAKE_SOURCE_DIR}")

message(STATUS "Source dir: ${CMAKE_SOURCE_DIR}")
message(STATUS "Binary dir: ${CMAKE_BINARY_DIR}")

project(TRADE_DIVERGENCE_MODE_1)

set(CMAKE_CXX_STANDARD 20)

# add_executable(999_Learn main.cpp MyStudies.cpp)

add_library(TRADE_DIVERGENCE_MODE_1 SHARED Studies.cpp
        helpers.h
        helpers.cpp
        Studies.cpp
        Studies.cpp
        UpDownTickCost.cpp)

set_target_properties(TRADE_DIVERGENCE_MODE_1 PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "D:\\SierraChartCME\\Data"
)
