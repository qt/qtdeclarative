# Generated from graph.pro.

cmake_minimum_required(VERSION 3.14)
project(graph LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)

add_qt_gui_executable(graph
    graph.cpp graph.h
    gridnode.cpp gridnode.h
    linenode.cpp linenode.h
    main.cpp
    noisynode.cpp noisynode.h
)
target_link_libraries(graph PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
)

# Resources:
set(graph_resource_files
    "main.qml"
    "shaders/line.fsh"
    "shaders/line.vsh"
    "shaders/noisy.fsh"
    "shaders/noisy.vsh"
)

QT6_ADD_RESOURCES(graph "graph"
    PREFIX
        "/scenegraph/graph"
    FILES
        ${graph_resource_files}
)


install(TARGETS graph
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
