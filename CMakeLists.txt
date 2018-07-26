cmake_minimum_required(VERSION 2.8.12)

if (CMAKE_MAJOR_VERSION LESS 3)
    project(libECHMETUpdateCheck)
else ()
    project(libECHMETUpdateCheck LANGUAGES C CXX)
endif ()

option(EUPD_ENABLE_DIAGNOSTICS "Enable verbose diagnostic output" OFF)

add_definitions("-DECHMET_DLL_BUILD")

if (EUPD_ENABLE_DIAGNOSTICS)
    add_definitions("-DEUPD_ENABLE_DIAGNOSTICS")
endif ()

if (WIN32)
    set("ECHMET_PLATFORM_WIN32" "1")

    if (MINGW)
        add_definitions("-DECHMET_COMPILER_MINGW")
    elseif (MSYS)
        add_definitions("-DECHMET_COMPILER_MSYS")
    elseif (MSVC)
        add_definitions("-DECHMET_COMPILER_MSVC")
        set(DEFAULT_SYMVER_LINK " ")
    else ()
        message(FATAL "Unsupported or mistedected compiler")
    endif ()
elseif (UNIX)
    set("ECHMET_PLATFORM_UNIX" "1")

    if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9.2)
            message(FATAL_ERROR "GCC 4.9.2 or newer is required to build libECHMETUpdateCheck")
	endif ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -std=c++14 -Wall -Wextra -pedantic -fvisibility=hidden -g")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -std=c89 -Wall -Wextra -pedantic -fvisibility=hidden -g")
        add_definitions("-DECHMET_COMPILER_GCC_LIKE")
        set(DEFAULT_SYMVER_LINK "-Wl,--default-symver")
    elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.4)
		message(FATAL_ERROR "Clang 3.4 or newer is required to build libECHMETUpdateCheck")
	endif ()

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -std=c++14 -Wall -Wextra -Wno-c++98-compat -pedantic -fvisibility=hidden -g")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -std=c89 -Wall -Wextra -pedantic -fvisibility=hidden -g")
        add_definitions("-DECHMET_COMPILER_GCC_LIKE")
        set(DEFAULT_SYMVER_LINK "-Wl,--default-symver")
    else ()
        message(FATAL "Unsupported or misdetected compiler")
    endif ()
endif ()

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/src/echmetupdatecheck_config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/echmetupdatecheck_config.h")

include_directories(${INCLUDE_DIRECTORIES}
                    "${ECHMET_CORE_LIBS_DIR}/include"
                    "${CMAKE_CURRENT_SOURCE_DIR}/include"
                    ${CMAKE_CURRENT_BINARY_DIR})

set(libECHMETUpdateCheck_SRCS
    src/update_check.c
    src/list_fetcher.c
    src/list_parser.cpp
    src/list_comparator.c)

set(EUPDCHK_LINK_LIBS
    curl)

add_library(ECHMETUpdateCheck SHARED ${libECHMETUpdateCheck_SRCS})
target_include_directories(ECHMETUpdateCheck PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
set_target_properties(ECHMETUpdateCheck
                      PROPERTIES VERSION 0.1
                                 SOVERSION 0.1
                                 LINK_FLAGS ${DEFAULT_SYMVER_LINK}
                      PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/echmetupdatecheck.h")
target_link_libraries(ECHMETUpdateCheck
                      PRIVATE ${EUPDCHK_LINK_LIBS})

if (WIN32)
    install(TARGETS ECHMETUpdateCheck
            RUNTIME DESTINATION lib
            ARCHIVE DESTINATION lib
            PUBLIC_HEADER DESTINATION include)
else ()
    install(TARGETS ECHMETUpdateCheck
            LIBRARY DESTINATION lib
            PUBLIC_HEADER DESTINATION include)
endif ()
if (WIN32)
    if (MINGW OR MSYS)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libECHMETUpdateCheck.lib" DESTINATION lib RENAME ECHMETUpdateCheck.lib)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libECHMETUpdateCheck.dll" DESTINATION lib RENAME ECHMETUpdateCheck.dll)
    endif ()
endif ()

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/echmetupdatecheck_config.h"
        DESTINATION include)
