cmake_minimum_required(VERSION 3.21)
project(wxlib)
set(CMAKE_CXX_STANDARD 23)

set(LIB_NAME ${PROJECT_NAME})

# Create target and add include path
add_library(${LIB_NAME} INTERFACE)
add_library(${LIB_NAME}::${LIB_NAME} ALIAS ${LIB_NAME})

add_subdirectory(meta_enum)
add_subdirectory(semimap)
add_subdirectory(mio)
add_subdirectory(msgpack)
add_subdirectory(matchit)

# Add include directory to library
set(INCLUDE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/meta_enum/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/semimap/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/mio/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/matchit/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/msgpack/include")

target_include_directories(
        ${PROJECT_NAME}
        INTERFACE
        $<BUILD_INTERFACE:${INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>)


