# CMake build script for the ETrun project

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project(ETrun VERSION 2.0.0 HOMEPAGE_URL "timeruns.net/ETrun" LANGUAGES C)
set(ETrun_VERSION ${PROJECT_VERSION})

configure_file(
    "${PROJECT_SOURCE_DIR}/src/game/bg_version.h.in"
    "${PROJECT_SOURCE_DIR}/src/game/bg_version.h"
)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/etrun)

option(CROSS_COMPILE32 "Compile 32-bit x86 version of ETrun" OFF)
option(BUILD_PK3 "Pack the mod libraries and game scripts into mod pk3" OFF)

#-----------------------------------------------------------------
# Architecture stuff
#-----------------------------------------------------------------
# Determine architecture of the machine
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ETRUN_ARCH_64 ON)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(ETRUN_ARCH_32 ON)
else()
    message(FATAL_ERROR "Unknown target architecture detected. Pointer size: ${CMAKE_SIZEOF_VOID_P}")
endif()

# Determine CPU of the machine (source: ET:Legacy)
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" system_name_lower)
if(system_name_lower MATCHES "(i386)|(i686)|(x86)|(amd64)")
	message(STATUS "x86 architecture detected")
	set(ETRUN_X86 1)
elseif(system_name_lower MATCHES "(arm)|(aarch64)")
	message(STATUS "ARM architecture detected")
	set(ETRUN_ARM 1)
else()
	message(WARNING "Unknown architecture detected: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# On Apple silicon, we can build for both arm64 and x86_64
if (APPLE AND ETRUN_ARM)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
    message(STATUS "Target architecture: arm64 and x86_64 (Universal binaries)")
endif()

if(CROSS_COMPILE32)
    if(ETRUN_ARM)
        message(FATAL_ERROR "Cross compiling is not supported for ARM!")
    endif()

    if(WIN32)
        set(ARCH_NAME "x86")
    elseif(APPLE)
        set(ARCH_NAME "")
        add_compile_options(-arch i386)
        add_link_options(-arch i386)
    else()
        set(ARCH_NAME "i386")
        add_compile_options(-m32)
        add_link_options(-m32)
    endif()

    add_compile_definitions(PRIVATE MMDB_UINT128_IS_BYTE_ARRAY=1)

    message(STATUS "Target architecture: x86")
else()
    # x86_64 build
    if (ETRUN_X86)
        if(WIN32)
            set(ARCH_NAME "x64")
        elseif(APPLE)
            set(ARCH_NAME "")
        else()
            set(ARCH_NAME "x86_64")
        endif()
    elseif(ETRUN_ARM)
        if(WIN32)
            message(FATAL_ERROR "Compiling for Windows ARM is not supported!")
        elseif(APPLE)
            set(ARCH_NAME "")
        else()
            set(ARCH_NAME "aarch64")
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
    PROPERTIES
    PREFIX ""
    C_STANDARD 99
)
target_compile_definitions(cgame PRIVATE CGAMEDLL=1)

add_library(qagame MODULE ${QAGAME_SRC})
set_target_properties(qagame
    PROPERTIES
    PREFIX ""
    C_STANDARD 99
)
target_compile_definitions(qagame PRIVATE GAMEDLL=1)

add_library(ui MODULE ${UI_SRC})
set_target_properties(ui
    PROPERTIES
    PREFIX ""
    C_STANDARD 99
)

#-----------------------------------------------------------------
# Platform-specific settings
#-----------------------------------------------------------------
if(UNIX)
    # On unix, use system threads
    find_package(Threads)

    # Strict mode
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        -Wshadow
        -Wcast-align
        -Wstrict-prototypes
        -Wmissing-prototypes
        -Wno-missing-field-initializers
    )

    if(CMAKE_SYSTEM MATCHES "OpenBSD*")
        set(OS_LIBRARIES m pthread)
        set(LIB_SUFFIX ".mp.obsd.")
    elseif(CMAKE_SYSTEM MATCHES "FreeBSD")
        set(OS_LIBRARIES m pthread)
        set(LIB_SUFFIX ".mp.fbsd.")
    elseif(CMAKE_SYSTEM MATCHES "NetBSD")
        set(OS_LIBRARIES m pthread)
        set(LIB_SUFFIX ".mp.nbsd.")
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
        add_subdirectory("${PROJECT_SOURCE_DIR}/libs/pthreads-win32")
        target_link_libraries(qagame pthreads4w)
        add_dependencies(qagame pthreads4w)

        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
        set(OS_LIBRARIES ws2_32 psapi winmm)
        set(LIB_SUFFIX "_mp_")

        # Set output path
        set_target_properties(cgame PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/etrun")
        set_target_properties(cgame PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/etrun")
        set_target_properties(qagame PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/etrun")
        set_target_properties(qagame PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/etrun")
        set_target_properties(ui PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/etrun")
        set_target_properties(ui PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/etrun")
    endif()
endif()

set_target_properties(cgame PROPERTIES OUTPUT_NAME cgame${LIB_SUFFIX}${ARCH_NAME})
set_target_properties(qagame PROPERTIES OUTPUT_NAME qagame${LIB_SUFFIX}${ARCH_NAME})
set_target_properties(ui PROPERTIES OUTPUT_NAME ui${LIB_SUFFIX}${ARCH_NAME})

#--------------
# Maxmind GeoIP
#--------------
set(BUILD_TESTING OFF)
set(MAXMINDDB_BUILD_BINARIES OFF)
set(MAXMINDDB_INSTALL OFF)
add_subdirectory("${PROJECT_SOURCE_DIR}/libs/libmaxminddb")
target_link_libraries(qagame maxminddb)
add_dependencies(qagame maxminddb)

#-----------------------------------------------------------------
# Pk3 (inspired from etlegacy https://github.com/etlegacy/etlegacy)
#-----------------------------------------------------------------
if(BUILD_PK3)
    find_program(ZIP_EXECUTABLE zip PATHS ${CMAKE_CURRENT_BINARY_DIR})
    if(NOT ZIP_EXECUTABLE AND WIN32)
        message(STATUS "Downloading zip.exe to " ${CMAKE_CURRENT_BINARY_DIR}/zip.exe)
        file(DOWNLOAD http://stahlworks.com/dev/zip.exe ${CMAKE_CURRENT_BINARY_DIR}/zip.exe SHOW_PROGRESS TIMEOUT 10)
        find_program(ZIP_EXECUTABLE zip PATHS ${CMAKE_CURRENT_BINARY_DIR})
    endif()

    add_custom_target(pk3 ALL DEPENDS cgame ui)
    set_target_properties(pk3 PROPERTIES OUTPUT_NAME ETrun-${ETrun_VERSION}.pk3)
    SET(CLIENT_ASSETS_PATH etrun/)
    add_custom_command(TARGET pk3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/${CLIENT_ASSETS_PATH} ${CMAKE_CURRENT_BINARY_DIR}/etrun/)
    add_custom_command(TARGET pk3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/etrun/custommapscripts)
    add_custom_command(TARGET pk3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_BINARY_DIR}/etrun/ETrun-${ETrun_VERSION}.pk3)
    if (WIN32)
        add_custom_command(TARGET pk3 POST_BUILD
            COMMAND ${ZIP_EXECUTABLE} -q -r ETrun-${ETrun_VERSION}.pk3 * -x qagame${LIB_SUFFIX}${ARCH_NAME}* -x *.pdb -x *.ilk
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/etrun
        )
    else()
        add_custom_command(TARGET pk3 POST_BUILD
            COMMAND ${ZIP_EXECUTABLE} -q -r ETrun-${ETrun_VERSION}.pk3 *
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/etrun
        )
    endif(WIN32)
endif()
