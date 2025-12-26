cmake_minimum_required(VERSION 3.22)

project(uartl VERSION 0.1.0 LANGUAGES C)

add_library(uartl STATIC)

target_include_directories(uartl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_subdirectory(src)

target_compile_options(uartl PRIVATE
	-Wall
	-Wextra
	-pedantic
)
