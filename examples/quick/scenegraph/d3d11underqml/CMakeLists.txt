# Generated from d3d11underqml.pro.

cmake_minimum_required(VERSION 3.14)
project(d3d11underqml LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

add_qt_gui_executable(d3d11underqml
    d3d11squircle.cpp d3d11squircle.h
    main.cpp
)
target_link_libraries(d3d11underqml PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
    d3d11
    d3dcompiler
)

# Resources:
set(d3d11underqml_resource_files
    "main.qml"
    "squircle.frag"
    "squircle.vert"
)

QT6_ADD_RESOURCES(d3d11underqml "d3d11underqml"
    PREFIX
        "/scenegraph/d3d11underqml"
    FILES
        ${d3d11underqml_resource_files}
)


install(TARGETS d3d11underqml
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
