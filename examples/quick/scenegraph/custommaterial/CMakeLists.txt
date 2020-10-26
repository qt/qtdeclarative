# Generated from custommaterial.pro.

cmake_minimum_required(VERSION 3.14)
project(custommaterial_declarative LANGUAGES CXX) # special case

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/custommaterial")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

qt_add_executable(custommaterial_declarative # special case
    customitem.cpp customitem.h
    main.cpp
)
set_target_properties(custommaterial_declarative PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(custommaterial_declarative PUBLIC # special case
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(custommaterial_resource_files
    "main.qml"
    "shaders/mandelbrot.frag.qsb"
    "shaders/mandelbrot.vert.qsb"
)

qt6_add_resources(custommaterial_declarative "custommaterial" # special case
    PREFIX
        "/scenegraph/custommaterial"
    FILES
        ${custommaterial_resource_files}
)

install(TARGETS custommaterial_declarative # special case
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(custommaterial_declarative PROPERTIES # special case
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI ExampleCustomMaterial
)

qt6_qml_type_registration(custommaterial_declarative) # special case
