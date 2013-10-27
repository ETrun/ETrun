#
# ETrun CMakeLists.txt
#

cmake_minimum_required (VERSION 2.6)

project (ETrun C)

find_package (Threads)

set (ETrun_VERSION "1.1.0")

configure_file (
	"${PROJECT_SOURCE_DIR}/src/game/bg_version.h.in"
	"${PROJECT_SOURCE_DIR}/src/game/bg_version.h"
)
include_directories("${PROJECT_SOURCE_DIR}/src")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/etrun)

# has to be set to "", otherwise CMake will pass -rdynamic resulting in a client crash
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

# Strict mode
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-self-assign -Werror")

# Check if a target architecture was given in CMake invocation
if("${TARGET_ARCHITECTURE}" STREQUAL "")
	set(TARGET_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Print architecture we are going to use
message(STATUS "#############################################")
message(STATUS "###       Target architecture: ${TARGET_ARCHITECTURE}       ###")
message(STATUS "#############################################")

# Get the system architecture
if("${TARGET_ARCHITECTURE}" STREQUAL "i686" OR "${TARGET_ARCHITECTURE}" STREQUAL "i386" OR "${TARGET_ARCHITECTURE}" STREQUAL "x86")
	if(WIN32)
		set(ARCH "x86")
	elseif(NOT CMAKE_SYSTEM MATCHES "Darwin")
		set(ARCH "i386")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
	else()
		set(ARCH "")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
	endif()
elseif("${TARGET_ARCHITECTURE}" STREQUAL "x86_64" OR "${TARGET_ARCHITECTURE}" STREQUAL "AMD64")
	if(WIN32)
		set(ARCH "x86_64")
	elseif(NOT CMAKE_SYSTEM MATCHES "Darwin")
		set(ARCH "x86_64")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
	else()
		set(ARCH "")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
	endif()
else()
	set(ARCH "${TARGET_ARCHITECTURE}")
	message(STATUS "Warning: processor architecture not recognised (${TARGET_ARCHITECTURE})")
endif()

#-----------------------------------------------------------------
# Platform-specific settings
#-----------------------------------------------------------------

if(UNIX)
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
	# Test&fix this
	add_definitions(-DWINVER=0x501)
	set(OS_LIBRARIES ws2_32 psapi winmm)
	set(LIB_SUFFIX "_mp_")
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

# qagame requires pthreads but OSX already includes pthread, so linking is not needed on this OS
if(NOT CMAKE_SYSTEM MATCHES "Darwin")
	target_link_libraries (qagame${LIB_SUFFIX}${ARCH} "pthread -D_REENTRANT")
endif()

#
# ui
#
add_library(ui${LIB_SUFFIX}${ARCH} MODULE ${UI_SRC})
set_target_properties(ui${LIB_SUFFIX}${ARCH}
	PROPERTIES
	PREFIX ""
)
