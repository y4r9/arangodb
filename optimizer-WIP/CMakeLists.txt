
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

set_target_properties(arango_optimizer PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${GTEST_INCLUDES}"
)
set_target_properties(optimizer-test PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${GTEST_INCLUDES}"
)


add_dependencies(optimizer-test arango_aql)
add_dependencies(optimizer-test arango_optimizer)
add_dependencies(optimizer-test gtest)

target_link_libraries(optimizer-test arango)
target_link_libraries(optimizer-test arango_aql)
target_link_libraries(optimizer-test gtest)
target_link_libraries(optimizer-test Threads::Threads)
