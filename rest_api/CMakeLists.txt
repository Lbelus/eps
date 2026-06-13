cmake_minimum_required(VERSION 3.22)
project(rest_api VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_GTEST "Enable Google Test" OFF)
option(ENABLE_BASIC_FLAGS "compile with basic flags option" OFF)
option(ENABLE_FULL_FLAGS "compile with advanced flags option" OFF)
option(ENABLE_BETA_FLAGS "compile with beta flags option" OFF)
option(ENABLE_SHARED "Enable shared lib and package install" OFF)
option(ENABLE_REDIS "Enable REDIS DB" ON)
option(ENABLE_MYSQLPP "Enable MYSQL_DB" ON)

function(build_redispp_lib target_name)
    if (ENABLE_REDIS)
# Find and link hiredis library
        find_path(HIREDIS_HEADER hiredis)
        target_include_directories(${target_name} PUBLIC ${HIREDIS_HEADER})
        find_library(HIREDIS_LIB hiredis)
        target_link_libraries(${target_name} PRIVATE ${HIREDIS_LIB})

# Find and link redis-plus-plus library
        find_path(REDIS_PLUS_PLUS_HEADER sw)
        target_include_directories(${target_name} PUBLIC ${REDIS_PLUS_PLUS_HEADER})
        find_library(REDIS_PLUS_PLUS_LIB redis++)
        target_link_libraries(${target_name} PRIVATE ${REDIS_PLUS_PLUS_LIB})
    endif()
endfunction()


function(build_mysqlpp_lib target_name)
    if (ENABLE_MYSQLPP)
        find_library(MYSQLPP_LIB mysqlpp REQUIRED)
        find_library(MYSQLCLIENT_LIB mysqlclient REQUIRED)
        target_link_libraries(${target_name}
              PRIVATE
                  ${MYSQLPP_LIB}
                  ${MYSQLCLIENT_LIB}
        )
    endif()
endfunction()


function(build_dbs target_name)
    build_mysqlpp_lib(${target_name})
    build_redispp_lib(${target_name})
endfunction()

function(compile_with_gtest)
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
        )
        include(GoogleTest)
        gtest_discover_tests(repo_tests)
    endif()
endfunction()

function(compile_with_basic_flags)
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
            Threads::Threads
        )
    endif()
endfunction()

function(compile_with_full_flags)
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
            Threads::Threads
        )
    endif()
endfunction()

function(compile_with_beta_flags)
    if (ENABLE_BETA_FLAGS)
        target_compile_options(rest_apiObj PRIVATE 
        $<$<COMPILE_LANGUAGE:CXX>: -g -Wall -Wextra -Wpedantic -O2 -DNDEBUG >
        )
        target_compile_options(rest_api PRIVATE 
        $<$<COMPILE_LANGUAGE:CXX>: -g -Wall -Wextra -Wpedantic -O2 -DNDEBUG >
        )
        target_link_options(rest_api PRIVATE -fsanitize=address)
        target_compile_definitions(rest_api PRIVATE REPO_ENABLE_FAKE=0 REPO_FAKE_ONLY=0)
        target_link_libraries(
            rest_api
            PRIVATE
            rest_apiObj
            Threads::Threads
        )
    endif()
endfunction()

function(compile_project)
    compile_with_gtest()
    compile_with_basic_flags()
    compile_with_full_flags()
    compile_with_beta_flags()
endfunction()

if (NOT ENABLE_SHARED)

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

    build_dbs(rest_apiObj)

    add_executable(rest_api src/main.cpp)
    compile_project()
endif()

if (ENABLE_SHARED)
    include(GNUInstallDirs)
    find_package(Threads REQUIRED)
    add_library(rest_apiLib SHARED
        src/core/rest_api.cpp
        src/core/mysql_conn_pool.cpp
        src/core/config_loader.cpp
    )
    set_target_properties(rest_apiLib PROPERTIES OUTPUT_NAME rest_api)
    target_compile_options(rest_apiLib PRIVATE -fPIC)

    build_dbs(rest_apiLib)
    target_include_directories(rest_apiLib
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    target_link_libraries(rest_apiLib
        PUBLIC
            Threads::Threads
    )


    set(CMAKE_INSTALL_LIBDIR lib) # default lib
    set(CMAKE_INSTALL_INCLUDEDIR include) # default include
    set(PREFIX ${CMAKE_INSTALL_PREFIX})
    set(EXEC_PREFIX "\${prefix}")
    set(LIBDIR "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
    set(INCLUDEDIR "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/pkg-config.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
        @ONLY
    )

    install(TARGETS rest_apiLib
            EXPORT ${PROJECT_NAME}Targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
    install(EXPORT ${PROJECT_NAME}Targets
            FILE ${PROJECT_NAME}Targets.cmake
            NAMESPACE ${PROJECT_NAME}::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        "${PROJECT_NAME}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    )
    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
endif()
