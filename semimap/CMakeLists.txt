add_executable(semimap_test semimap_test.cpp)
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}")
target_include_directories(semimap_test PRIVATE ${INCLUDE_DIR})
