# -*- mode: CMAKE; -*-

project(rocksdb C CXX)

set(FAIL_ON_WARNINGS OFF CACHE BOOL "do not enable -Werror")

set(ARANGO_AKUMULI_VERSION "6d91b2b")
set(ARANGO_AKUMULI_VERSION "${ARANGO_AKUMULI_VERSION}" PARENT_SCOPE)

add_definitions(-DBOOST_SPIRIT_THREADSAFE)
add_definitions(-DBOOST_PHOENIX_THREADSAFE)
add_definitions(-DBOOST_DATE_TIME_POSIX_TIME_STD_CONFIG)

find_package(APR REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories("${APR_INCLUDE_DIR}")
include_directories("${APRUTIL_INCLUDE_DIR}")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${ARANGO_AKUMULI_VERSION}/include)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/${ARANGO_AKUMULI_VERSION}/libakumuli EXCLUDE_FROM_ALL)

target_link_libraries(akumuli)
# result
list(APPEND LINK_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/akumuli/${ARANGO_AKUMULI_VERSION}")
