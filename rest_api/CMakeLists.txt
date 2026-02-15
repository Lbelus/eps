cmake_minimum_required(VERSION 3.22)
project(rest_api VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
file(GLOB_RECURSE SRC_DIRS LIST_DIRECTORIES true "${SRC_DIR}/*")
list(FILTER SRC_DIRS EXCLUDE REGEX "\\.[^.]+$")

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    /usr/include/mysql
    ${SRC_DIRS}
)

file(GLOB_RECURSE SOURCES "src/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX ".*main\\.cpp$")

add_library(rest_apiObj OBJECT ${SOURCES})

find_package(Threads REQUIRED)

find_package(nlohmann_json 3.11.3 REQUIRED)
target_link_libraries(rest_apiObj PRIVATE nlohmann_json::nlohmann_json)

find_library(MYSQLPP_LIB mysqlpp REQUIRED)
find_library(MYSQLCLIENT_LIB mysqlclient REQUIRED)

# Find and link hiredis library
find_path(HIREDIS_HEADER hiredis)
target_include_directories(rest_apiObj PUBLIC ${HIREDIS_HEADER})
find_library(HIREDIS_LIB hiredis)
target_link_libraries(rest_apiObj PRIVATE ${HIREDIS_LIB})

# Find and link redis-plus-plus library
find_path(REDIS_PLUS_PLUS_HEADER sw)
target_include_directories(rest_apiObj PUBLIC ${REDIS_PLUS_PLUS_HEADER})
find_library(REDIS_PLUS_PLUS_LIB redis++)
target_link_libraries(rest_apiObj PRIVATE ${REDIS_PLUS_PLUS_LIB})

add_executable(rest_api src/main.cpp)

option(ENABLE_GTEST "Enable Google Test" OFF)

if(ENABLE_GTEST)
    enable_testing()
    find_package(GTest REQUIRED)
    add_executable(repo_tests tests/test_example_repo.cc)
    target_compile_definitions(repo_tests PRIVATE REPO_ENABLE_FAKE=1 REPO_FAKE_ONLY=1)
    target_link_libraries(
        repo_tests 
        PRIVATE 
        GTest::gtest_main 
        Threads::Threads
        ${MYSQLPP_LIB}
    )
    include(GoogleTest)
    gtest_discover_tests(repo_tests)
endif()


if (ENABLE_BASIC_FLAGS)
    target_compile_options(rest_apiObj PRIVATE 
    $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Werror -fsanitize=address >
    )
    target_compile_options(rest_api PRIVATE 
    $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Werror -fsanitize=address >
    )
    target_link_options(rest_api PRIVATE -fsanitize=address)
    target_compile_definitions(rest_api PRIVATE REPO_ENABLE_FAKE=0 REPO_FAKE_ONLY=0)
    target_link_libraries(
        rest_api
        PRIVATE
        rest_apiObj
        ${MYSQLPP_LIB}
        ${MYSQLCLIENT_LIB}
        Threads::Threads
    )
endif()
if (ENABLE_FULL_FLAGS)
    target_compile_options(rest_apiObj PRIVATE 
    $<$<COMPILE_LANGUAGE:CXX>:-g3 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wno-unused-parameter -Wno-snig-conversion -fsanitize=address >
    )
    target_compile_options(rest_api PRIVATE 
    $<$<COMPILE_LANGUAGE:CXX>:-g3 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wno-unused-parameter -Wno-snig-conversion -fsanitize=address >
    )
    target_link_options(rest_api PRIVATE -fsanitize=address)
    target_compile_definitions(rest_api PRIVATE REPO_ENABLE_FAKE=0 REPO_FAKE_ONLY=0)
    target_link_libraries(
        rest_api
        PRIVATE
        rest_apiObj
        ${MYSQLPP_LIB}
        ${MYSQLCLIENT_LIB}
        Threads::Threads
    )
endif()

