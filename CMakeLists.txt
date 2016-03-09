CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(telebot C)

SET(PROJECT_DESCRIPTION "Telegram Bot API")
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "${PREFIX}/bin")
SET(INCLUDEDIR "${PREFIX}/include/${PROJECT_NAME}")
SET(LIBDIR "${PREFIX}/lib")
SET(VERSION 0.1.0)

SET(CMAKE_MACOSX_RPATH 1)

SET(SRCS
    src/telebot-parser.c
    src/telebot-core-api.c
    src/telebot-api.c
)

ADD_DEFINITIONS("-DDEBUG=1")
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)
SET(DEPENDENTS "libcurl json-c")
INCLUDE(FindPkgConfig)
pkg_check_modules(PKGS REQUIRED ${DEPENDENTS})

FOREACH(flag ${PKGS_CFLAGS})
    SET(EXTRA_LIB_CFLAGS "${EXTRA_LIB_CFLAGS} ${flag}")
ENDFOREACH(flag)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_LIB_CFLAGS} -Werror -Wall" )

# libtelebot
ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PKGS_LDFLAGS})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES VERSION ${VERSION})

# package configuration
CONFIGURE_FILE(telebot.pc.in telebot.pc @ONLY)

# echobot (test)
ADD_SUBDIRECTORY(test)

# CMake Policy (CMP0002)
# The logical name of executable and library targets
# does not have to correspond to the physical file name built.
INSTALL(FILES telebot.pc DESTINATION lib/pkgconfig)
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION lib COMPONENT RuntimeLibraries)
INSTALL(FILES
    include/telebot-api.h
    include/telebot-common.h
    include/telebot-core-api.h
    DESTINATION include/telebot/)

