cmake_minimum_required(VERSION 3.12)

project(arango_optimizer)

add_subdirectory(3rdParty/patterns)

FILE(GLOB optimizer_test_files "src/tests/*.cpp")

add_library(arango_optimizer OBJECT ${optimizer_test_files})

add_executable(optimizer-test
  src/optimizer-test.cpp
  $<TARGET_OBJECTS:arango_optimizer>
)

target_include_directories(optimizer-test PRIVATE
  ./src/
)

target_include_directories(arango_optimizer SYSTEM PRIVATE
  ${CMAKE_SOURCE_DIR}/3rdParty/rocksdb/${ARANGO_ROCKSDB_VERSION}/third-party/gtest-1.8.1/fused-src
)
target_include_directories(optimizer-test SYSTEM PRIVATE
  ${CMAKE_SOURCE_DIR}/3rdParty/rocksdb/${ARANGO_ROCKSDB_VERSION}/third-party/gtest-1.8.1/fused-src
)

target_link_libraries(arango_optimizer arango)
target_link_libraries(arango_optimizer arango_aql)
target_link_libraries(arango_optimizer gtest)
target_link_libraries(arango_optimizer immer)
target_link_libraries(arango_optimizer mpark_patterns)
target_link_libraries(optimizer-test arango)
target_link_libraries(optimizer-test arango_aql)
target_link_libraries(optimizer-test gtest)
target_link_libraries(optimizer-test Threads::Threads)
