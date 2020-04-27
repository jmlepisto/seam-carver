cmake_minimum_required(VERSION 2.8)
project(seam-carver)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

find_package(OpenMP)
find_package(OpenCV REQUIRED)

# Static linking for OpenCV

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(
carver
src/carver.cpp
src/main.cpp
)

message("OpenCV_Shared=${OpenCV_SHARED}")

target_link_libraries(carver ${OpenCV_LIBS})
if(OpenMP_CXX_FOUND)
    target_link_libraries(carver OpenMP::OpenMP_CXX)
endif()

