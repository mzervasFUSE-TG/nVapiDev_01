cmake_minimum_required(VERSION 3.15)
project(nvapi_timings_070124)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR}/nvapi)

add_executable(nvapi_timings_070124 src/nvapi_timings_070124.cpp)

target_link_libraries(nvapi_timings_070124 PRIVATE ${CMAKE_SOURCE_DIR}/nvapi/amd64/nvapi64.lib)





