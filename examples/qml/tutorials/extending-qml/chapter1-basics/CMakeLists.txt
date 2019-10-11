# Generated from chapter1-basics.pro.

cmake_minimum_required(VERSION 3.14)
project(chapter1-basics LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/qml/tutorials/extending-qml/chapter1-basics")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

add_qt_gui_executable(chapter1-basics
    main.cpp
    piechart.cpp piechart.h
)
target_link_libraries(chapter1-basics PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(chapter1-basics_resource_files
    "app.qml"
)

qt6_add_resources(chapter1-basics "chapter1-basics"
    PREFIX
        "/"
    FILES
        ${chapter1-basics_resource_files}
)

install(TARGETS chapter1-basics
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
