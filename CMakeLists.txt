cmake_minimum_required(VERSION 2.8)
project(seam-carver)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(
carver
src/carver.cpp
src/main.cpp
)

target_link_libraries(carver ${OpenCV_LIBS})
