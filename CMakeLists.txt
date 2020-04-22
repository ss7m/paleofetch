cmake_minimum_required(VERSION 3.13.0)

project(paleofetch
	VERSION 0.0.0
	DESCRIPTION "neofetch, but written in C"
	HOMEPAGE_URL "https://github.com/sam-barr/paleofetch"
	LANGUAGES "C"
)

# Without this, paths are not relative in the sources list
cmake_policy(SET CMP0076 NEW)

include(GNUInstallDirs)
include(FindPkgConfig)

# The program itself
add_executable(${PROJECT_NAME})

# Sources list
add_subdirectory(src)

# Libraries
pkg_check_modules(X11 REQUIRED x11)

# Header path
target_include_directories(${PROJECT_NAME}
	PUBLIC
		${X11_INCLUDE_DIRS}
)

# Linking
target_link_libraries(${PROJECT_NAME}
	PUBLIC
		${X11_LIBRARIES}
)

# Other flags
target_compile_definitions(${PROJECT_NAME}
	PUBLIC
		${X11_CFLAGS_OTHER}
)

# Default flags
if(NOT DEFINED ENV{CFLAGS})
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -g")
endif()


install(TARGETS ${PROJECT_NAME}
	DESTINATION ${CMAKE_INSTALL_BINDIR}
)
