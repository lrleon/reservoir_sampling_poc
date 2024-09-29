cmake_minimum_required(VERSION 3.29)

project(median_poc)

set(CMAKE_CXX_STANDARD 20)
include_directories(.)
include_directories(Aleph-w)
link_directories(Aleph-w)

add_executable(reservoir_sampling reservoir_sampling.cc)
target_link_libraries(reservoir_sampling PRIVATE Aleph gsl)
