# Generated from openglunderqml.pro.

cmake_minimum_required(VERSION 3.14)
project(openglunderqml LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

add_qt_gui_executable(openglunderqml
    main.cpp
    squircle.cpp squircle.h
)
target_link_libraries(openglunderqml PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

# Resources:
set(openglunderqml_resource_files
    "main.qml"
)

QT6_ADD_RESOURCES(openglunderqml "openglunderqml"
    PREFIX
        "/scenegraph/openglunderqml"
    FILES
        ${openglunderqml_resource_files}
)


install(TARGETS openglunderqml
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
