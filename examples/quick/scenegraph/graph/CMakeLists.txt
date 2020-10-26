# Generated from graph.pro.

cmake_minimum_required(VERSION 3.14)
project(graph LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/graph")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)

qt_add_executable(graph
    graph.cpp graph.h
    gridnode.cpp gridnode.h
    linenode.cpp linenode.h
    main.cpp
    noisynode.cpp noisynode.h
)
set_target_properties(graph PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(graph PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
)


# Resources:
set(graph_resource_files
    "main.qml"
    "shaders/line.frag.qsb"
    "shaders/line.vert.qsb"
    "shaders/noisy.frag.qsb"
    "shaders/noisy.vert.qsb"
)

qt6_add_resources(graph "graph"
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

set_target_properties(graph PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI Graph
)

qt6_qml_type_registration(graph)
