# Generated from qquickviewcomparison.pro.

cmake_minimum_required(VERSION 3.16)
project(qquickviewcomparison LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/quickwidgets/qquickviewcomparison")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS QuickWidgets)
find_package(Qt6 COMPONENTS ShaderTools)

qt_add_executable(qquickviewcomparison
    fbitem.cpp fbitem.h
    logo.cpp logo.h
    main.cpp
    mainwindow.cpp mainwindow.h
)
set_target_properties(qquickviewcomparison PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(qquickviewcomparison PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::QuickWidgets
    Qt::Widgets
)

qt_add_qml_module(qquickviewcomparison
    URI fbitem
    VERSION 1.0
    QML_FILES test.qml
    RESOURCE_PREFIX /qquickviewcomparison
    NO_RESOURCE_TARGET_PATH
)

qt6_add_shaders(qquickviewcomparison "shaders"
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/qquickviewcomparison"
    FILES
        "wobble.frag"
)

install(TARGETS qquickviewcomparison
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
