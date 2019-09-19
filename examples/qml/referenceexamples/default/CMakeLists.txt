# Generated from default.pro.

cmake_minimum_required(VERSION 3.14)
project(default LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Qml)

add_qt_gui_executable(default
    birthdayparty.cpp birthdayparty.h
    main.cpp
    person.cpp person.h
)
target_link_libraries(default PUBLIC
    Qt::Core
    Qt::Qml
)

# Resources:
set(default_resource_files
    "example.qml"
)

qt6_add_resources(default "default"
    PREFIX
        "/"
    FILES
        ${default_resource_files}
)


install(TARGETS default
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
