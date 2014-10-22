cmake_minimum_required(VERSION 2.8)
project(3DTiles)
include(ExternalProject)

set(3DTiles_INSTALL_DIR "${3DTiles_SOURCE_DIR}/bin")
# Use modified FindSDL2 and FindGLEW that will work with my windows setup
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${3DTiles_SOURCE_DIR}/cmake")

set(stb_image_INCLUDE_DIR "${3DTiles_SOURCE_DIR}/external/stb_image/include")
file(DOWNLOAD "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h"
	"${stb_image_INCLUDE_DIR}/stb_image.h")

# Bump up warning levels appropriately for each compiler
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -std=c++14")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -ggdb -DDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -m64 -O3 -march=native")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /Zi")
	endif()
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -DDEBUG")
endif()

add_definitions(-DGLM_FORCE_RADIANS)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
# On windows we need to find GLM too
if (WIN32)
	find_package(GLM REQUIRED)
	include_directories(${GLM_INCLUDE_DIRS})
endif()

include_directories(include ${SDL2_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR}
	${stb_image_INCLUDE_DIR})
add_subdirectory(src)

