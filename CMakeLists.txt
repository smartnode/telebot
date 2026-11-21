CMAKE_MINIMUM_REQUIRED(VERSION 3.20)

# Sources shared by every platform
SET(TELEBOT_COMMON_SRCS
    src/telebot-parser.c
    src/telebot-core.c
    src/telebot.c
    src/telebot-inline.c
    src/telebot-forums.c
    src/telebot-payments.c
    src/telebot-passport.c
    src/telebot-games.c
    src/telebot-transport.c
    src/telebot-multipart.c
    src/telebot-request.c
    src/telebot-arena.c
    src/telebot-json.c
    src/telebot-core-messages.c
    src/telebot-core-members.c
)

# ---------------------------------------------------------------------------
# ESP-IDF component build. This file doubles as the component's CMakeLists.txt
# when the repository is added via EXTRA_COMPONENT_DIRS or the component manager.
# ---------------------------------------------------------------------------
IF(ESP_PLATFORM)
    # JSON comes from the vendored cJSON (ESP-IDF v6 no longer ships one).
    idf_component_register(
        SRCS ${TELEBOT_COMMON_SRCS}
             src/transport/telebot-transport-esp.c
             third_party/cjson/cJSON.c
        INCLUDE_DIRS include
        PRIV_INCLUDE_DIRS include_private third_party/cjson
        PRIV_REQUIRES esp_http_client esp-tls mbedtls esp_hw_support)
    TARGET_COMPILE_DEFINITIONS(${COMPONENT_LIB} PRIVATE _GNU_SOURCE)
    SET_SOURCE_FILES_PROPERTIES(third_party/cjson/cJSON.c PROPERTIES COMPILE_OPTIONS "-Wno-error")
    RETURN()
ENDIF()

# ---------------------------------------------------------------------------
# Host build (Linux / macOS)
# ---------------------------------------------------------------------------
PROJECT(telebot C)

# C11 (gnu11): _Generic and _Static_assert are used internally
SET(CMAKE_C_STANDARD 11)
SET(CMAKE_C_STANDARD_REQUIRED ON)
SET(CMAKE_C_EXTENSIONS ON)

SET(PROJECT_DESCRIPTION "Telegram Bot API")
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "${PREFIX}/bin")
SET(INCLUDEDIR "${PREFIX}/include/${PROJECT_NAME}")
SET(LIBDIR "${PREFIX}/lib")
SET(VERSION 10.4)

SET(CMAKE_MACOSX_RPATH 1)

SET(SRCS ${TELEBOT_COMMON_SRCS})

# HTTP transport backend
SET(TELEBOT_TRANSPORT "curl" CACHE STRING "HTTP transport backend (curl)")
IF(TELEBOT_TRANSPORT STREQUAL "curl")
    LIST(APPEND SRCS src/transport/telebot-transport-curl.c)
ELSE()
    MESSAGE(FATAL_ERROR "Unsupported TELEBOT_TRANSPORT: ${TELEBOT_TRANSPORT}")
ENDIF()

# JSON: always the vendored cJSON in third_party/cjson. To use a different
# cJSON version, replace cJSON.c and cJSON.h there.
SET(DEPENDENTS "libcurl")
LIST(APPEND SRCS third_party/cjson/cJSON.c)
# Third-party code: keep its warnings visible but never fatal
SET_SOURCE_FILES_PROPERTIES(third_party/cjson/cJSON.c PROPERTIES
    COMPILE_OPTIONS "-Wno-error;-Wno-deprecated-declarations")
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/third_party/cjson)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include_private)
INCLUDE(FindPkgConfig)
pkg_check_modules(PKGS REQUIRED ${DEPENDENTS})

FOREACH(flag ${PKGS_CFLAGS})
    SET(EXTRA_LIB_CFLAGS "${EXTRA_LIB_CFLAGS} ${flag}")
ENDFOREACH(flag)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_LIB_CFLAGS} -D_GNU_SOURCE -Werror -Wall" )

#Debug option
IF(DEBUG)
    MESSAGE("Build debug version")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
    ADD_DEFINITIONS("-DDEBUG")
ELSE(DEBUG)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
ENDIF(DEBUG)

# libtelebot
ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS})
ADD_LIBRARY(${PROJECT_NAME}_static STATIC ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PKGS_LDFLAGS})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES VERSION ${VERSION})

# package configuration
CONFIGURE_FILE(telebot.pc.in telebot.pc @ONLY)

# Unit tests (GoogleTest, no network needed)
OPTION(TELEBOT_BUILD_TESTS "Build GoogleTest unit tests" ON)
IF(TELEBOT_BUILD_TESTS)
    ENABLE_TESTING()
    ADD_SUBDIRECTORY(test)
ENDIF()

# Host examples (the ESP32 example is a separate ESP-IDF project)
OPTION(TELEBOT_BUILD_EXAMPLES "Build host examples" ON)
IF(TELEBOT_BUILD_EXAMPLES)
    ADD_SUBDIRECTORY(examples/host_demo_bot)
ENDIF()

# CMake Policy (CMP0002)
# The logical name of executable and library targets
# does not have to correspond to the physical file name built.
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/telebot.pc DESTINATION lib/pkgconfig)
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION lib COMPONENT RuntimeLibraries)
INSTALL(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/telebot.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/telebot-common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/telebot-methods.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/telebot-types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/telebot-core.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/telebot-stickers.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/telebot-inline.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/telebot-forums.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/telebot-payments.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/telebot-passport.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/telebot-games.h
    DESTINATION include/telebot/)

