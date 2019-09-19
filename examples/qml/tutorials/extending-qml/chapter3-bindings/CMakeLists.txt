# Generated from chapter3-bindings.pro.

cmake_minimum_required(VERSION 3.14)
project(chapter3-bindings LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

add_qt_gui_executable(chapter3-bindings
    main.cpp
    piechart.cpp piechart.h
)
target_link_libraries(chapter3-bindings PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

# Resources:
set(chapter3-bindings_resource_files
    "app.qml"
)

qt6_add_resources(chapter3-bindings "chapter3-bindings"
    PREFIX
        "/"
    FILES
        ${chapter3-bindings_resource_files}
)


install(TARGETS chapter3-bindings
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
