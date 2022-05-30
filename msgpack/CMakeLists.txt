add_executable(msgpack_test msgpack_test.cpp)

set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}")
target_include_directories(msgpack_test PRIVATE ${INCLUDE_DIR})