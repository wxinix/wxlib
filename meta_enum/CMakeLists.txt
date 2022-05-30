add_executable(meta_enum_test meta_enum_test.cpp)
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}")
target_include_directories(meta_enum_test PRIVATE ${INCLUDE_DIR})