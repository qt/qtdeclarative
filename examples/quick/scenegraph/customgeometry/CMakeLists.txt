# Generated from customgeometry.pro.

cmake_minimum_required(VERSION 3.16)
project(customgeometry_declarative LANGUAGES CXX) # special case

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/customgeometry")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)

qt_add_executable(customgeometry_declarative # special case
    beziercurve.cpp beziercurve.h
    main.cpp
)
set_target_properties(customgeometry_declarative PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(customgeometry_declarative PUBLIC # special case
    Qt::Core
    Qt::Gui
    Qt::Quick
)

qt_add_qml_module(customgeometry_declarative
    URI CustomGeometry
    VERSION 1.0
    QML_FILES main.qml
    RESOURCE_PREFIX /scenegraph/customgeometry
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS customgeometry_declarative # special case
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
