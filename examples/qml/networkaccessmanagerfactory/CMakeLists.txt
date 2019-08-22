# Generated from networkaccessmanagerfactory.pro.

cmake_minimum_required(VERSION 3.14)
project(networkaccessmanagerfactory LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Network)

add_qt_gui_executable(networkaccessmanagerfactory
    main.cpp
)
target_link_libraries(networkaccessmanagerfactory PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Network
    Qt::Qml
    Qt::Quick
)

# Resources:
set(networkaccessmanagerfactory_resource_files
    "view.qml"
)

QT6_ADD_RESOURCES(networkaccessmanagerfactory "networkaccessmanagerfactory"
    PREFIX
        "/"
    FILES
        ${networkaccessmanagerfactory_resource_files}
)


install(TARGETS networkaccessmanagerfactory
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
