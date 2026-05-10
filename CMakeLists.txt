cmake_minimum_required(VERSION 3.20)

project(JettGraphicsLibrary VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(JETT_BUILD_SHARED "Build the graphics library as a shared library" ON)
option(JETT_BUILD_TESTS "Build the legacy GraphicsTest executable" OFF)

find_package(Freetype REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(TIFF REQUIRED)
find_package(OpenMP)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LCMS2 REQUIRED IMPORTED_TARGET lcms2)

set(JETT_LIBRARY_TYPE STATIC)
if(JETT_BUILD_SHARED)
    set(JETT_LIBRARY_TYPE SHARED)
endif()

set(JETT_LIBRARY_SOURCES
    GraphicsLibrary/Bitblt.cpp
    GraphicsLibrary/CLKernel.cpp
    GraphicsLibrary/CLProgram.cpp
    GraphicsLibrary/CPUBitblt.cpp
    GraphicsLibrary/CPUDither.cpp
    GraphicsLibrary/CPULine.cpp
    GraphicsLibrary/CPUPolygon.cpp
    GraphicsLibrary/CPUProcessor.cpp
    GraphicsLibrary/Font.cpp
    GraphicsLibrary/Image.cpp
    GraphicsLibrary/jett.cpp
    GraphicsLibrary/jett_exception.cpp
    GraphicsLibrary/jett_image.cpp
    GraphicsLibrary/jett_matrix.cpp
    GraphicsLibrary/k_only_transform_sampler.cpp
    GraphicsLibrary/lcm_transform_sampler.cpp
    GraphicsLibrary/Line.cpp
    GraphicsLibrary/Linearization.cpp
    GraphicsLibrary/LinearizationCollection.cpp
    GraphicsLibrary/OpenCLRuntime.cpp
    GraphicsLibrary/OrderedBitmap.cpp
    GraphicsLibrary/OrderedScreen.cpp
    GraphicsLibrary/OrderedScreenCollection.cpp
    GraphicsLibrary/transform.cpp
    GraphicsLibrary/stdafx.cpp
)

if(WIN32)
    list(APPEND JETT_LIBRARY_SOURCES GraphicsLibrary/gdi_font_sampler.cpp)
else()
    list(APPEND JETT_LIBRARY_SOURCES
        GraphicsLibrary/GPUBitblt.cpp
        GraphicsLibrary/GPUDither.cpp
        GraphicsLibrary/GPUPolygon.cpp
        GraphicsLibrary/GPUProcessor.cpp
        GraphicsLibrary/GPUWorker.cpp
        GraphicsLibrary/freetype_font_sampler.cpp
    )
endif()

add_library(jett ${JETT_LIBRARY_TYPE} ${JETT_LIBRARY_SOURCES})

target_include_directories(jett
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/GraphicsLibrary
        ${FREETYPE_INCLUDE_DIRS}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(jett
    PUBLIC
        Freetype::Freetype
        JPEG::JPEG
        PNG::PNG
        TIFF::TIFF
        PkgConfig::LCMS2
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(jett PUBLIC OpenMP::OpenMP_CXX)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(jett PUBLIC dl)
endif()

target_compile_definitions(jett PRIVATE JETT_LIBRARY_BUILD=1)

if(JETT_BUILD_TESTS)
    add_executable(GraphicsTest
        GraphicsTest/colour_management_tests.cpp
        GraphicsTest/main.cpp
        GraphicsTest/timer.cpp
    )
    target_include_directories(GraphicsTest PRIVATE GraphicsLibrary GraphicsTest)
    target_link_libraries(GraphicsTest PRIVATE jett)
endif()