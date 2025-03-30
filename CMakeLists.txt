cmake_minimum_required(VERSION 3.10)
project(AsciiVision)

# Find OpenCV package
find_package(OpenCV REQUIRED)

# Add the include directory for your own header files (e.g., ascii_filter.hpp)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add OpenCV include directories
include_directories(${OpenCV_INCLUDE_DIRS})

# Add your source files
add_executable(AsciiVision main.cpp src/ascii_filter.cpp)

# Link OpenCV libraries
target_link_libraries(AsciiVision ${OpenCV_LIBS})
