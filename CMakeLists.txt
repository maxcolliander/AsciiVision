cmake_minimum_required(VERSION 3.10)
project(AsciiVision)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(AsciiVision src/main.cpp src/ascii_filter.cpp)

target_link_libraries(AsciiVision ${OpenCV_LIBS})
