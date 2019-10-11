# Generated from shell.pro.

cmake_minimum_required(VERSION 3.14)
project(shell LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/qml/shell")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Qml)

add_qt_gui_executable(shell
    main.cpp
)
target_link_libraries(shell PUBLIC
    Qt::Core
    Qt::Qml
)

install(TARGETS shell
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
