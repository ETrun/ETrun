# CMake build script for the ETrun project

cmake_minimum_required(VERSION 2.6)
project(ETrun C)
set(ETrun_VERSION "1.2.1")

configure_file(
	"${PROJECT_SOURCE_DIR}/src/game/bg_version.h.in"
	"${PROJECT_SOURCE_DIR}/src/game/bg_version.h"
)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/etrun)

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
)

#-----------------------------------------------------------------
# Architecture stuff
#-----------------------------------------------------------------
# Determine architecture of the machine
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(ETRUN_ARCH_64 ON)
	message(STATUS "Current architecture: x86_64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(ETRUN_ARCH_32 ON)
    message(STATUS "Current architecture: x86")
else()
	set(ETRUN_ARCH_32 ON)
    message(STATUS "Unsupported architecture")
endif()

# Check if we should build OSX universal binaries
if(OSX_UNIVERSAL_BINARIES)
	set(ARCH "")
	set_target_properties(cgame PROPERTIES OSX_ARCHITECTURES "i386;x86_64")
	set_target_properties(qagame PROPERTIES OSX_ARCHITECTURES "i386;x86_64")
	set_target_properties(ui PROPERTIES OSX_ARCHITECTURES "i386;x86_64")
	message(STATUS "Target architecture: x86 and x86_64 (Universal binaries)")
else()
	# Check if a target architecture was specified
	if("${TARGET_ARCHITECTURE}" STREQUAL "x86")
		set(ETRUN_ARCH_32 ON)
	elseif("${TARGET_ARCHITECTURE}" STREQUAL "x86_64")
		set(ETRUN_ARCH_64 ON)
	endif()

	if(ETRUN_ARCH_32)
		# x86 build
		if(WIN32)
	        set(ARCH "x86")
	    elseif(APPLE)
			set(ARCH "")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch i386")
	    else()
	        set(ARCH "i386")
	        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
	    endif()
	    message(STATUS "Target architecture: x86")
	else()
		# x86_64 build
		if(WIN32)
	        set(ARCH "x86_64")
	    elseif(APPLE)
			set(ARCH "")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch x86_64")
	    else()
	        set(ARCH "x86_64")
	        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
	    endif()
	    message(STATUS "Target architecture: x86_64")
	endif()
endif()

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
	target_link_libraries(qagame ${OS_LIBRARIES})
elseif(WIN32)
	if(MSVC)
		# Link with pthreads-win32
		set(PTHREADS_BUILD_STATIC 1)
		add_subdirectory("${PROJECT_SOURCE_DIR}/libs/pthreads-win32")
		include_directories("${PROJECT_SOURCE_DIR}/libs/pthreads-win32/pthreads.2")
		target_link_libraries(qagame debug "${CMAKE_BINARY_DIR}/libs/pthreads-win32/Debug/pthreadsVC2d.lib")
		target_link_libraries(qagame optimized "${CMAKE_BINARY_DIR}/libs/pthreads-win32/Release/pthreadsVC2.lib")
		add_definitions(-DPTW32_STATIC_LIB)
		add_dependencies(qagame pthreadsVC2)

		add_definitions(-D_CRT_SECURE_NO_WARNINGS)
		set(OS_LIBRARIES ws2_32 psapi winmm)
		set(LIB_SUFFIX "_mp_")

		# Set outpout path
		set_target_properties(cgame PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/etrun")
		set_target_properties(cgame PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/etrun")
		set_target_properties(qagame PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/etrun")
		set_target_properties(qagame PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/etrun")
		set_target_properties(ui PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/etrun")
		set_target_properties(ui PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/etrun")
	endif()
endif()

set_target_properties(cgame PROPERTIES OUTPUT_NAME cgame${LIB_SUFFIX}${ARCH})
set_target_properties(qagame PROPERTIES OUTPUT_NAME qagame${LIB_SUFFIX}${ARCH})
set_target_properties(ui PROPERTIES OUTPUT_NAME ui${LIB_SUFFIX}${ARCH})

#-----------------------------------------------------------------
# Pk3 (inspired from etlegacy https://github.com/etlegacy/etlegacy)
#-----------------------------------------------------------------
find_program(ZIP_EXECUTABLE zip PATHS ${CMAKE_CURRENT_BINARY_DIR})
if(NOT ZIP_EXECUTABLE AND WIN32)
	message(STATUS "Downloading zip.exe to " ${CMAKE_CURRENT_BINARY_DIR}/zip.exe)
	file(DOWNLOAD http://stahlworks.com/dev/zip.exe ${CMAKE_CURRENT_BINARY_DIR}/zip.exe SHOW_PROGRESS TIMEOUT 10)
	find_program(ZIP_EXECUTABLE zip PATHS ${CMAKE_CURRENT_BINARY_DIR})
endif()

add_custom_target(pk3 ALL DEPENDS cgame ui)
set_target_properties(pk3 PROPERTIES OUTPUT_NAME ETrun-${ETrun_VERSION}.pk3)
SET(CLIENT_ASSETS_PATH etrun/)
add_custom_command(TARGET pk3
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/${CLIENT_ASSETS_PATH} ${CMAKE_CURRENT_BINARY_DIR}/etrun/)
add_custom_command(TARGET pk3
	COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/etrun/custommapscripts)
add_custom_command(TARGET pk3
	COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_BINARY_DIR}/etrun/ETrun-${ETrun_VERSION}.pk3)
if (WIN32)
	add_custom_command(TARGET pk3
		COMMAND ${ZIP_EXECUTABLE} -q -r ETrun-${ETrun_VERSION}.pk3 * -x qagame${LIB_SUFFIX}${ARCH}* -x *.pdb -x *.ilk
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/etrun
	)
else()
	add_custom_command(TARGET pk3
		COMMAND ${ZIP_EXECUTABLE} -q -r ETrun-${ETrun_VERSION}.pk3 *
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/etrun
	)
endif(WIN32)
