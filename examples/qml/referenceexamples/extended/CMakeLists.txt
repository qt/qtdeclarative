# Generated from extended.pro.

cmake_minimum_required(VERSION 3.14)
project(extended LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(extended
    lineedit.cpp lineedit.h
    main.cpp
)
target_link_libraries(extended PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Widgets
)

# Resources:
set(extended_resource_files
    "example.qml"
)

QT6_ADD_RESOURCES(extended "extended"
    PREFIX
        "/"
    FILES
        ${extended_resource_files}
)


install(TARGETS extended
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
