CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(telebot C)

SET(SRCS
    src/telebot-parser.c
    src/telebot-core-api.c
    src/telebot-api.c
)

ADD_DEFINITIONS("-DDEBUG=1")
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)

INCLUDE(FindPkgConfig)
pkg_check_modules(libpkgs REQUIRED
	libcurl
	json-c)

FOREACH(flag ${libpkgs_CFLAGS})
	SET(EXTRA_LIB_CFLAGS "${EXTRA_LIB_CFLAGS} ${flag}")
ENDFOREACH(flag)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_LIB_CFLAGS}")

# libtelebot
ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${libpkgs_LDFLAGS})
#SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES VERSION ${VERSION})

# CMake Policy (CMP0002)
# The logical name of executable and library targes
# does not have to correspond to the physical file name built.
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION lib COMPONENT RuntimeLibraries)
