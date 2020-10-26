# Generated from openglunderqml.pro.

cmake_minimum_required(VERSION 3.14)
project(openglunderqml LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/openglunderqml")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

qt_add_executable(openglunderqml
    main.cpp
    squircle.cpp squircle.h
)
set_target_properties(openglunderqml PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(openglunderqml PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(openglunderqml_resource_files
    "main.qml"
)

qt6_add_resources(openglunderqml "openglunderqml"
    PREFIX
        "/scenegraph/openglunderqml"
    FILES
        ${openglunderqml_resource_files}
)

install(TARGETS openglunderqml
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(openglunderqml PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI OpenGLUnderQML
)

qt6_qml_type_registration(openglunderqml)
