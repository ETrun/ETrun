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
	set(ETRUN_ARCH_64 ON)
	message(STATUS "Current architecture: x86_64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(ETRUN_ARCH_32 ON)
    message(STATUS "Current architecture: x86")
else()
    message(FATAL_ERROR "Unsupported architecture")
endif()

# Check if a target architecture was given in CMake invocation
if(APPLE)
	# Build universal binaries
	set(ARCH "")
	message(STATUS "Target architecture: x86 and x86_64 (Universal binaries)")
else()
	if("${TARGET_ARCHITECTURE}" STREQUAL "")
		if(ETRUN_ARCH_32)
			# x86 build
			if(WIN32)
		            set(ARCH "x86")
		    else()
		            set(ARCH "i386")
		            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
		    endif()
		    message(STATUS "Target architecture: x86")
		else()
			# x86_64 build
			if(WIN32)
		            set(ARCH "x86_64")
		    else()
		            set(ARCH "x86_64")
		            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
		    endif()
		    message(STATUS "Target architecture: x86_64")
		endif()
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
	OSX_ARCHITECTURES "i386;x86_64"
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

#-----------------------------------------------------------------
# Pk3 (from etlegacy)
#-----------------------------------------------------------------
find_program(ZIP_EXECUTABLE zip PATHS ${CMAKE_BINARY_DIR})
if(NOT ZIP_EXECUTABLE AND WIN32)
	message(STATUS "Downloading zip.exe to " ${CMAKE_BINARY_DIR}/zip.exe)
	file(DOWNLOAD http://stahlworks.com/dev/zip.exe ${CMAKE_BINARY_DIR}/zip.exe SHOW_PROGRESS TIMEOUT 10)
	find_program(ZIP_EXECUTABLE zip PATHS ${CMAKE_BINARY_DIR})
endif()
find_program(UNZIP_EXECUTABLE unzip PATHS ${CMAKE_BINARY_DIR})
if(NOT UNZIP_EXECUTABLE AND WIN32)
	message(STATUS "Downloading unzip.exe to " ${CMAKE_BINARY_DIR}/unzip.exe)
	file(DOWNLOAD http://stahlworks.com/dev/unzip.exe ${CMAKE_BINARY_DIR}/unzip.exe SHOW_PROGRESS TIMEOUT 10)
	find_program(UNZIP_EXECUTABLE unzip PATHS ${CMAKE_BINARY_DIR})
endif()
add_custom_target(pk3 ALL DEPENDS etrun/ETrun-${ETrun_VERSION}.pk3)
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/etrun/ETrun-${ETrun_VERSION}.pk3
	COMMAND ${ZIP_EXECUTABLE} ETrun-${ETrun_VERSION}.pk3 cgame${LIB_SUFFIX}${ARCH}* ui${LIB_SUFFIX}${ARCH}*
	DEPENDS cgame ui
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/etrun/
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/etrun/ETrun-${ETrun_VERSION}.pk3
	DESTINATION "${INSTALL_DEFAULT_MODDIR}/etrun"
)
