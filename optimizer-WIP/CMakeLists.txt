add_library(arango_optimizer OBJECT
  src/tests/test.cpp
)

add_dependencies(arango_optimizer gtest)

add_executable(optimizer-test
  src/optimizer-test.cpp
  $<TARGET_OBJECTS:arango_optimizer>
)

target_include_directories(optimizer-test PUBLIC
  ./src/
)

target_include_directories(arango_optimizer SYSTEM PRIVATE
  ${CMAKE_SOURCE_DIR}/3rdParty/rocksdb/${ARANGO_ROCKSDB_VERSION}/third-party/gtest-1.8.1/fused-src
)
target_include_directories(optimizer-test SYSTEM PRIVATE
  ${CMAKE_SOURCE_DIR}/3rdParty/rocksdb/${ARANGO_ROCKSDB_VERSION}/third-party/gtest-1.8.1/fused-src
)

add_dependencies(optimizer-test gtest)
add_dependencies(arango_optimizer gtest)
add_dependencies(arango_optimizer arango)
add_dependencies(arango_optimizer arango_aql)

target_link_libraries(optimizer-test arango)
target_link_libraries(optimizer-test arango_aql)
target_link_libraries(optimizer-test gtest)
target_link_libraries(optimizer-test Threads::Threads)
