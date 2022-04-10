cmake_minimum_required(VERSION 2.8.12)

if (CMAKE_MAJOR_VERSION LESS 3)
    project(libECHMETUpdateCheck)
else ()
    project(libECHMETUpdateCheck LANGUAGES C CXX)
endif ()

option(EUPD_ENABLE_DIAGNOSTICS "Enable verbose diagnostic output" OFF)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if (UNIX)
    include(GNUInstallDirs)
elseif (WIN32)
    set(CMAKE_INSTALL_LIBDIR "lib")
    set(CMAKE_INSTALL_INCLUDEDIR "include")
    set(CMAKE_INSTALL_BINDIR "bin")
endif ()

add_definitions("-DECHMET_DLL_BUILD")

if (EUPD_ENABLE_DIAGNOSTICS)
    add_definitions("-DEUPD_ENABLE_DIAGNOSTICS")
endif ()

if (WIN32)
    option(EUPD_EXTERNAL_CURL "Use custom path to libcurl library" ON)

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
    option(EUPD_EXTERNAL_CURL "Use custom path to libcurl library" OFF)

    set("ECHMET_PLATFORM_UNIX" "1")

    if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9.2)
            message(FATAL_ERROR "GCC 4.9.2 or newer is required to build libECHMETUpdateCheck")
	endif ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -Wall -Wextra -pedantic -fvisibility=hidden -g")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -Wall -Wextra -pedantic -fvisibility=hidden -g")
        add_definitions("-DECHMET_COMPILER_GCC_LIKE")
        set(DEFAULT_SYMVER_LINK "-Wl,--default-symver")
    elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.4)
            message(FATAL_ERROR "Clang 3.4 or newer is required to build libECHMETUpdateCheck")
	endif ()

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -Wall -Wextra -Wno-c++98-compat -pedantic -fvisibility=hidden -g")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -Wall -Wextra -pedantic -fvisibility=hidden -g")
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

if (EUPD_EXTERNAL_CURL)
    link_directories("${LIBCURL_DIR}/lib")
    include_directories(${INCLUDE_DIRECTORIES}
                        "${LIBCURL_DIR}/include")
endif ()

if (WIN32)
    set(EUPDCHK_LINK_LIBS
        libcurl)
else ()
    set(EUPDCHK_LINK_LIBS
        curl)
endif ()

add_library(ECHMETUpdateCheck SHARED ${libECHMETUpdateCheck_SRCS})
target_include_directories(ECHMETUpdateCheck PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
set_target_properties(ECHMETUpdateCheck
                      PROPERTIES VERSION 0.1
                                 SOVERSION 0.1
                                 LINK_FLAGS ${DEFAULT_SYMVER_LINK}
                      PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/echmetupdatecheck.h")
target_link_libraries(ECHMETUpdateCheck
                      PRIVATE ${EUPDCHK_LINK_LIBS})

install(TARGETS ECHMETUpdateCheck
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/echmetupdatecheck_config.h"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
