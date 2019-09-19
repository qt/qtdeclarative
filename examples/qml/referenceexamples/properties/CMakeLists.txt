# Generated from properties.pro.

cmake_minimum_required(VERSION 3.14)
project(properties LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Qml)

add_qt_gui_executable(properties
    birthdayparty.cpp birthdayparty.h
    main.cpp
    person.cpp person.h
)
target_link_libraries(properties PUBLIC
    Qt::Core
    Qt::Qml
)

# Resources:
set(properties_resource_files
    "example.qml"
)

qt6_add_resources(properties "properties"
    PREFIX
        "/"
    FILES
        ${properties_resource_files}
)


install(TARGETS properties
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
