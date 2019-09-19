# Generated from simplematerial.pro.

cmake_minimum_required(VERSION 3.14)
project(simplematerial LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)

add_qt_gui_executable(simplematerial
    simplematerial.cpp
)
target_link_libraries(simplematerial PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
)

# Resources:
set(simplematerial_resource_files
    "main.qml"
)

qt6_add_resources(simplematerial "simplematerial"
    PREFIX
        "/scenegraph/simplematerial"
    FILES
        ${simplematerial_resource_files}
)


install(TARGETS simplematerial
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
