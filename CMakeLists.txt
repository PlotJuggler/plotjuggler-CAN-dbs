cmake_minimum_required(VERSION 3.7)

project(plotjuggler_can_plugins)

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE AND EXISTS /usr/local/opt/qt5)
    # Homebrew installs Qt5 (up to at least 5.9.1) in
    # /usr/local/qt5, ensure it can be found by CMake since
    # it is not in the default /usr/local prefix.
    # source: https://github.com/Homebrew/homebrew-core/issues/8392#issuecomment-325226494
    list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt5")
    set(CMAKE_MACOSX_RPATH 1)
endif()

find_package(Qt5 REQUIRED COMPONENTS
    Core
    Widgets
    Concurrent
    Xml
    Svg
)
find_package(Qt5 OPTIONAL_COMPONENTS
    SerialBus
)
find_package(PlotJuggler REQUIRED)

include_directories(
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Concurrent_INCLUDE_DIRS}
    ${Qt5Xml_INCLUDE_DIRS}
    ${PlotJuggler_INCLUDE_DIRS}
    ${Qt5Svg_INCLUDE_DIRS}
    ./3rdparty/dbcppp/include
)

set(PJ_LIBRARIES
    Qt5::Core
    Qt5::Widgets
    Qt5::Xml
    Qt5::Concurrent
    Qt5::Svg
    ${PlotJuggler_LIBRARY}
)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

add_subdirectory(3rdparty/dbcppp)

add_library(CanFrameProcessor STATIC
    PluginsCommonCAN/CanFrameProcessor.cpp
    PluginsCommonCAN/N2kMsg/GenericFastPacket.c
)
target_link_libraries(CanFrameProcessor
    ${PJ_LIBRARIES}
    libdbcppp
)

add_library(DataLoadCAN SHARED
    DataLoadCAN/dataload_can.h
    DataLoadCAN/dataload_can.cpp)

target_link_libraries(DataLoadCAN
    ${PJ_LIBRARIES}
    CanFrameProcessor
)
install(TARGETS DataLoadCAN DESTINATION bin)

# Build&Install DataStreamCAN only if SerialBus found.
if(${Qt5SerialBus_FOUND})
    message("-- Found Qt5::SerialBus, building DataStreamCAN.")
    add_library(DataStreamCAN SHARED
        DataStreamCAN/datastream_can.h
        DataStreamCAN/datastream_can.cpp
        DataStreamCAN/connectdialog.h
        DataStreamCAN/connectdialog.cpp)
    target_link_libraries(DataStreamCAN
        ${PJ_LIBRARIES}
        CanFrameProcessor
        Qt5::SerialBus
    )
    install(TARGETS DataStreamCAN DESTINATION bin)
else()
    message("-- Since Qt5::SerialBus not found, cannot build DataStreamCAN.")
endif()
