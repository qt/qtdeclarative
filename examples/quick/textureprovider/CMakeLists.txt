# Generated from textureprovider.pro.

cmake_minimum_required(VERSION 3.14)
project(textureprovider LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

add_qt_gui_executable(textureprovider
    etcprovider.cpp etcprovider.h
    main.cpp
)
target_link_libraries(textureprovider PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

# Resources:
set(textureprovider_resource_files
    "Label.qml"
    "images/qt-logo.jpg"
    "images/qt-logo.pkm"
    "textureprovider.qml"
)

qt6_add_resources(textureprovider "textureprovider"
    PREFIX
        "/textureprovider"
    FILES
        ${textureprovider_resource_files}
)


install(TARGETS textureprovider
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
