# Generated from methods.pro.

cmake_minimum_required(VERSION 3.14)
project(methods LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Qml)

add_qt_gui_executable(methods
    birthdayparty.cpp birthdayparty.h
    main.cpp
    person.cpp person.h
)
target_link_libraries(methods PUBLIC
    Qt::Core
    Qt::Qml
)

# Resources:
set(methods_resource_files
    "example.qml"
)

QT6_ADD_RESOURCES(methods "methods"
    PREFIX
        "/"
    FILES
        ${methods_resource_files}
)


install(TARGETS methods
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
