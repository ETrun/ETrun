#
# ETrun CMakeLists.txt
#
#


cmake_minimum_required (VERSION 2.6)

project (ETrun C)

find_package (Threads)

set (ETrun_VERSION "1.0.0")

configure_file (
	"${PROJECT_SOURCE_DIR}/src/game/bg_version.h.in"
	"${PROJECT_SOURCE_DIR}/src/game/bg_version.h"
)
include_directories("${PROJECT_SOURCE_DIR}/src")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(Build)

# Verbose output
set(CMAKE_VERBOSE_MAKEFILE "0")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/etrun)

# Used to store real system processor when we overwrite CMAKE_SYSTEM_PROCESSOR for cross-compile builds
set(SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR}) # See IS_64_BIT_BUILD_SYSTEM

# has to be set to "", otherwise CMake will pass -rdynamic resulting in a client crash
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")


#-----------------------------------------------------------------
# Platform-specific settings
#-----------------------------------------------------------------

if(UNIX)

	# optimization/debug flags
	# Note/FIXME: check this for Darwin, explore Win optimizations options
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s -ffast-math")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -Werror")

	if(CMAKE_SYSTEM MATCHES "OpenBSD*")
		set(LIB_SUFFIX ".mp.obsd.")
	elseif(CMAKE_SYSTEM MATCHES "Darwin")
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
	add_definitions(-DWINVER=0x501)
	set(OS_LIBRARIES ws2_32 psapi winmm)
	set(LIB_SUFFIX "_mp_")
endif()

# Get the system architecture
if(NOT CMAKE_SYSTEM MATCHES "Darwin")
	if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i686" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i386" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86")
		if(WIN32)
			set(ARCH "x86")
		else()
			set(ARCH "i386")
		endif()
	elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "AMD64")
		set(ARCH "x86_64")
	else()
		set(ARCH "${CMAKE_SYSTEM_PROCESSOR}")
		message(STATUS "Warning: processor architecture not recognised (${CMAKE_SYSTEM_PROCESSOR})")
	endif()
endif(NOT CMAKE_SYSTEM MATCHES "Darwin")

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
	"src/game/q_math.c"
	"src/game/q_shared.c"
	"src/ui/ui_shared.c"
	"libs/sha-1/sha1.c"
)

FILE(GLOB QAGAME_SRC
	"src/game/*.c"
	"src/game/*.h"
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
add_custom_command(
	TARGET cgame${LIB_SUFFIX}${ARCH} POST_BUILD
	COMMAND ./makebundle.sh ${CMAKE_BINARY_DIR}/etrun/cgame${LIB_SUFFIX}${ARCH}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

#
# qagame
#
add_library(qagame${LIB_SUFFIX}${ARCH} MODULE ${QAGAME_SRC})
set_target_properties(qagame${LIB_SUFFIX}${ARCH}
	PROPERTIES COMPILE_DEFINITIONS "GAMEDLL"
	PREFIX ""
)

# qagame requires pthreads but OSX already includes pthread, so linking is not needed on this OS
if(NOT CMAKE_SYSTEM MATCHES "Darwin")
	target_link_libraries (qagame${LIB_SUFFIX}${ARCH} "pthread -D_REENTRANT")
endif()

add_custom_command(
	TARGET qagame${LIB_SUFFIX}${ARCH} POST_BUILD
	COMMAND ./makebundle.sh ${CMAKE_BINARY_DIR}/etrun/qagame${LIB_SUFFIX}${ARCH}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

#
# ui
#
add_library(ui${LIB_SUFFIX}${ARCH} MODULE ${UI_SRC})
set_target_properties(ui${LIB_SUFFIX}${ARCH}
	PROPERTIES
	PREFIX ""
)
add_custom_command(
	TARGET ui${LIB_SUFFIX}${ARCH} POST_BUILD
	COMMAND ./makebundle.sh ${CMAKE_BINARY_DIR}/etrun/ui${LIB_SUFFIX}${ARCH}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)