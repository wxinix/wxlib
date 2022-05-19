add_executable(matchit_test matchit_test.cpp)

set(INCLUDE_DIR
        "${CMAKE_SOURCE_DIR}/doctest"
        "${CMAKE_SOURCE_DIR}/matchit")

target_include_directories(matchit_test PRIVATE ${INCLUDE_DIR})

add_subdirectory(sample)