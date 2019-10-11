# Generated from customgeometry.pro.

cmake_minimum_required(VERSION 3.14)
project(customgeometry LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/quick/scenegraph/customgeometry")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)

add_qt_gui_executable(customgeometry
    beziercurve.cpp beziercurve.h
    main.cpp
)
target_link_libraries(customgeometry PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
)


# Resources:
set(customgeometry_resource_files
    "main.qml"
)

qt6_add_resources(customgeometry "customgeometry"
    PREFIX
        "/scenegraph/customgeometry"
    FILES
        ${customgeometry_resource_files}
)

install(TARGETS customgeometry
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
