add_executable(mio_test mio_test.cpp)

if (MSVC)
    target_compile_options(mio_test PRIVATE /arch:AVX2)
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif ()

set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}")

target_include_directories(mio_test PRIVATE ${INCLUDE_DIR})