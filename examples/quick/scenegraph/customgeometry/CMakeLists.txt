# Generated from customgeometry.pro.

cmake_minimum_required(VERSION 3.14)
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


# Resources:
set(customgeometry_resource_files
    "main.qml"
)

qt6_add_resources(customgeometry_declarative "customgeometry" # special case
    PREFIX
        "/scenegraph/customgeometry"
    FILES
        ${customgeometry_resource_files}
)

install(TARGETS customgeometry_declarative # special case
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(customgeometry_declarative PROPERTIES # special case
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI CustomGeometry
)

qt6_qml_type_registration(customgeometry_declarative) # special case
