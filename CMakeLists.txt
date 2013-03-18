cmake_minimum_required(VERSION 2.6)
project(Poluplink C)

enable_testing()

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_subdirectory(lib)
add_subdirectory(cli)
