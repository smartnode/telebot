# GoogleTest unit tests. They need no network and no bot token.
ENABLE_LANGUAGE(CXX)
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

FIND_PACKAGE(GTest 1.11 QUIET)
IF(NOT GTest_FOUND)
    INCLUDE(FetchContent)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz
        URL_HASH SHA256=7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    SET(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    SET(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    SET(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
ENDIF()

ADD_EXECUTABLE(telebot_unit_tests
    fake_transport.cpp
    core_requests_test.cpp
    members_test.cpp
    messages_test.cpp
    multipart_test.cpp
    parser_test.cpp
    recent_api_test.cpp
    request_test.cpp
    transport_test.cpp
)
# The tests reach into private headers (parser, transport, json accessors).
TARGET_INCLUDE_DIRECTORIES(telebot_unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/third_party/cjson)
TARGET_LINK_LIBRARIES(telebot_unit_tests PRIVATE ${PROJECT_NAME}_static ${PKGS_LDFLAGS} GTest::gtest_main)

INCLUDE(GoogleTest)
GTEST_DISCOVER_TESTS(telebot_unit_tests)

# Every function declared in a public header must exist in the library
ADD_TEST(NAME api_symbols
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/api_symbols_test.sh $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_SOURCE_DIR}/include)
