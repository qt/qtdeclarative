# Generated from d3d11underqml.pro.

cmake_minimum_required(VERSION 3.14)
project(d3d11underqml LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/d3d11underqml")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

qt_add_executable(d3d11underqml
    d3d11squircle.cpp d3d11squircle.h
    main.cpp
)
set_target_properties(d3d11underqml PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(d3d11underqml PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
    d3d11
    d3dcompiler
)


# Resources:
set(d3d11underqml_resource_files
    "main.qml"
    "squircle.frag"
    "squircle.vert"
)

qt6_add_resources(d3d11underqml "d3d11underqml"
    PREFIX
        "/scenegraph/d3d11underqml"
    FILES
        ${d3d11underqml_resource_files}
)

install(TARGETS d3d11underqml
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(d3d11underqml PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI D3D11UnderQML
)

qt6_qml_type_registration(d3d11underqml)
