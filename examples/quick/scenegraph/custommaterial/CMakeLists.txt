# Generated from custommaterial.pro.

cmake_minimum_required(VERSION 3.16)
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
find_package(Qt6 COMPONENTS ShaderTools)

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

qt_add_qml_module(custommaterial_declarative
    URI ExampleCustomMaterial
    VERSION 1.0
    QML_FILES
        main.qml
    RESOURCE_PREFIX /scenegraph/custommaterial
    NO_RESOURCE_TARGET_PATH
)

qt6_add_shaders(custommaterial_declarative "shaders"
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/scenegraph/custommaterial"
    FILES
        "shaders/mandelbrot.vert"
        "shaders/mandelbrot.frag"
)

install(TARGETS custommaterial_declarative # special case
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
