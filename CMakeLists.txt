cmake_minimum_required(VERSION 4.2)

project(uartl VERSION 0.1.0 LANGUAGES C)

add_library(uartl STATIC)

target_include_directories(uartl
	PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)
add_subdirectory(src)

target_compile_options(uartl PRIVATE
	-Wall
	-Wextra
	-pedantic
)


# Installation
include(CMakePackageConfigHelpers)
configure_package_config_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/uartlConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/uartlConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/uartl
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/uartlConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
)

install(
	TARGETS uartl
	EXPORT uartlTargets
)

include(GNUInstallDirs)

install(
	EXPORT uartlTargets
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/uartl
	NAMESPACE uartl::
)

install(
	DIRECTORY include/
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
	FILES
		cmake/uartlConfig.cmake
		${CMAKE_CURRENT_BINARY_DIR}/uartlConfigVersion.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/uartl
)
