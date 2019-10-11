# Generated from import.pro.

cmake_minimum_required(VERSION 3.14)
project(chartsplugin LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/qml/tutorials/extending-qml/chapter6-plugins/Charts")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

qt6_add_qml_module(chartsplugin
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../Charts"
    VERSION 1.0
    URI "Charts"
    INSTALL_LOCATION ${INSTALL_EXAMPLEDIR}
)

target_sources(chartsplugin PRIVATE
    chartsplugin.cpp chartsplugin.h
    piechart.cpp piechart.h
    pieslice.cpp pieslice.h
)
target_link_libraries(chartsplugin PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

install(TARGETS chartsplugin
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
