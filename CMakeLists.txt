# CMake build script for the ETrun project

cmake_minimum_required(VERSION 2.6)
project(ETrun C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(ETrun_VERSION "1.1.1-DEV")

configure_file(
	"${PROJECT_SOURCE_DIR}/src/game/bg_version.h.in"
	"${PROJECT_SOURCE_DIR}/src/game/bg_version.h"
)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/etrun)

# Determine architecture of the machine
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	add_definitions(-DETRUN_ARCH_64)
	message(STATUS "Current architecture: x86_64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_definitions(-DETRUN_ARCH_32)
    message(STATUS "Current architecture: x86")
else()
    message(FATAL_ERROR "Unsupported architecture")
endif()

# Check if a target architecture was given in CMake invocation
if("${TARGET_ARCHITECTURE}" STREQUAL "")
	if("${DETRUN_ARCH_32}")
		# x86 build
		if(WIN32)
	            set(ARCH "x86")
	    elseif(APPLE)
	            set(ARCH "")
	    else()
	            set(ARCH "")
	            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
	    endif()
	    message(STATUS "Target architecture: x86")
	else()
		# x86_64 build
		if(WIN32)
	            set(ARCH "x86_64")
	    elseif(APPLE)
	            set(ARCH "")
	    else()
	            set(ARCH "")
	            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
	    endif()
	    message(STATUS "Target architecture: x86_64")
	endif()
endif()

#-----------------------------------------------------------------
# Sources
#-----------------------------------------------------------------
FILE(GLOB CGAME_SRC
	"src/cgame/*.c"
	"src/cgame/*.h"
	"src/game/bg_animation.c"
	"src/game/bg_animgroup.c"
	"src/game/bg_character.c"
	"src/game/bg_classes.c"
	"src/game/bg_misc.c"
  	"src/game/bg_pmove.c"
  	"src/game/bg_slidemove.c"
  	"src/game/bg_sscript.c"
	"src/game/bg_tracemap.c"
	"src/game/bg_weapons.c"
	"src/game/bg_version.h"
	"src/game/q_math.c"
	"src/game/q_shared.c"
	"src/ui/ui_shared.c"
	"libs/sha-1/sha1.c"
)

FILE(GLOB QAGAME_SRC
	"src/game/*.c"
	"src/game/*.h"
	"libs/geoip/geoip.c"
)

FILE(GLOB UI_SRC
	"src/ui/*.c"
	"src/ui/*.h"
	"src/game/q_math.c"
	"src/game/q_shared.c"
	"src/game/bg_classes.c"
	"src/game/bg_misc.c"
)

#-----------------------------------------------------------------
# Set targets
#-----------------------------------------------------------------
add_library(cgame MODULE ${CGAME_SRC})
set_target_properties(cgame
	PROPERTIES COMPILE_DEFINITIONS "CGAMEDLL"
	PREFIX ""
	OSX_ARCHITECTURES "i386;x86_64"
)

add_library(qagame MODULE ${QAGAME_SRC})
set_target_properties(qagame
	PROPERTIES COMPILE_DEFINITIONS "GAMEDLL"
	PREFIX ""
)

add_library(ui MODULE ${UI_SRC})
set_target_properties(ui
	PROPERTIES
	PREFIX ""
	OSX_ARCHITECTURES "i386;x86_64"
)

#-----------------------------------------------------------------
# Platform-specific settings
#-----------------------------------------------------------------
if(UNIX)
	# On unix, use system threads
	find_package(Threads)

	# Strict mode
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -std=c99 -pedantic")

	if(CMAKE_SYSTEM MATCHES "OpenBSD*")
		set(OS_LIBRARIES m pthread)
		set(LIB_SUFFIX ".mp.obsd.")
	elseif(CMAKE_SYSTEM MATCHES "FreeBSD")
		set(OS_LIBRARIES m pthread)
		set(LIB_SUFFIX ".mp.fbsd.")
	elseif(APPLE)
		set(OS_LIBRARIES dl m)
		set(LIB_SUFFIX "_mac")
		set(CMAKE_SHARED_MODULE_SUFFIX "")
	else()
		set(OS_LIBRARIES dl m rt pthread)
		set(LIB_SUFFIX ".mp.")
	endif()
elseif(WIN32)
	if(MSVC)
		# Link with pthreads-win32
		add_subdirectory("${PROJECT_SOURCE_DIR}/libs/pthreads-win32")
		include_directories("${PROJECT_SOURCE_DIR}/libs/pthreads-win32/pthreads.2")
		target_link_libraries(qagame debug "${CMAKE_BINARY_DIR}/libs/pthreads-win32/Debug/pthreadsVC2d.lib")
		target_link_libraries(qagame optimized "${CMAKE_BINARY_DIR}/libs/pthreads-win32/Release/pthreadsVC2.lib")

		add_definitions(-D_CRT_SECURE_NO_WARNINGS)
		set(OS_LIBRARIES ws2_32 psapi winmm)
		set(LIB_SUFFIX "_mp_")
	endif()
endif()

set_target_properties(cgame PROPERTIES OUTPUT_NAME cgame${LIB_SUFFIX}${ARCH})
set_target_properties(qagame PROPERTIES OUTPUT_NAME qagame${LIB_SUFFIX}${ARCH})
set_target_properties(ui PROPERTIES OUTPUT_NAME ui${LIB_SUFFIX}${ARCH})
