cmake_minimum_required(VERSION 2.6)
cmake_policy(VERSION 2.6)

project(SimpleHttpLib)
set(CMAKE_BUILD_TYPE, Release)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FILE_OFFSET_BITS=64 -Wall -Wextra -Wno-sign-compare -Werror=return-type -Wno-deprecated")

add_library(dxhttp SimpleHttp.cpp Utility.cpp SimpleHttpHeaders.cpp)
set_target_properties(dxhttp PROPERTIES COMPILE_FLAGS -fPIC)