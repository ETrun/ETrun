# CMake build script for the ETrun project
#
# Building (out of source build):
# > mkdir build && cd build
# > cmake .. [-DSETTINGS=VALUE]
# > cmake --build .

cmake_minimum_required(VERSION 2.6)
project(ETrun C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(ETrun_VERSION "1.1.1-DEV")

configure_file(
	"${PROJECT_SOURCE_DIR}/src/game/bg_version.h.in"
	"${PROJECT_SOURCE_DIR}/src/game/bg_version.h"
)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/etrun)

# has to be set to "", otherwise CMake will pass -rdynamic resulting in a client crash
# set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

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

#
# cgame
#
add_library(cgame${LIB_SUFFIX}${ARCH} MODULE ${CGAME_SRC})
set_target_properties(cgame${LIB_SUFFIX}${ARCH}
	PROPERTIES COMPILE_DEFINITIONS "CGAMEDLL"
	PREFIX ""
)

#
# qagame
#
add_library(qagame${LIB_SUFFIX}${ARCH} MODULE ${QAGAME_SRC})
set_target_properties(qagame${LIB_SUFFIX}${ARCH}
	PROPERTIES COMPILE_DEFINITIONS "GAMEDLL"
	PREFIX ""
)

#
# ui
#
add_library(ui${LIB_SUFFIX}${ARCH} MODULE ${UI_SRC})
set_target_properties(ui${LIB_SUFFIX}${ARCH}
	PROPERTIES
	PREFIX ""
)

#-----------------------------------------------------------------
# Platform-specific settings
#-----------------------------------------------------------------

if(UNIX)
	# On unix, use system threads
	find_package(Threads)

	# Strict mode
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-self-assign -Werror")

	if(CMAKE_SYSTEM MATCHES "OpenBSD*")
		set(LIB_SUFFIX ".mp.obsd.")
	elseif(APPLE)
		set(LIB_SUFFIX "_mac")
		set(CMAKE_SHARED_MODULE_SUFFIX "")
	else()
		set(LIB_SUFFIX ".mp.")
	endif()

	if(NOT MSYS)
		include(CheckCCompilerFlag)
		check_c_compiler_flag("-fvisibility=hidden" SUPPORT_VISIBILITY)
		if(SUPPORT_VISIBILITY)
			# note: visibility support starts with  __GNUC__ >= 4
			message(STATUS "Compiling with -fvisibility=hidden")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
		endif(SUPPORT_VISIBILITY)
	endif(NOT MSYS)

elseif(WIN32)
	if(MSVC)
		# Link with pthreads-win32
		add_subdirectory("${PROJECT_SOURCE_DIR}/libs/pthreads-win32")
		include_directories("${PROJECT_SOURCE_DIR}/libs/pthreads-win32/pthreads.2")
		target_link_libraries(qagame${LIB_SUFFIX}${ARCH} debug "${CMAKE_BINARY_DIR}/libs/pthreads-win32/Debug/pthreadsVC2d.lib")
		target_link_libraries(qagame${LIB_SUFFIX}${ARCH} optimized "${CMAKE_BINARY_DIR}/libs/pthreads-win32/Release/pthreadsVC2.lib")

		add_definitions(-D_CRT_SECURE_NO_WARNINGS)
		set(OS_LIBRARIES ws2_32 psapi winmm)
		set(LIB_SUFFIX "_mp_")
	endif()
endif()