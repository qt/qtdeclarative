# Generated from shadereffects.pro.

cmake_minimum_required(VERSION 3.14)
project(shadereffects LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/shadereffects")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(shadereffects
    main.cpp
)
set_target_properties(shadereffects PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(shadereffects PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)


# Resources:
set(shadereffects_resource_files
    "content/Slider.qml"
    "content/face-smile.png"
    "content/qt-logo.png"
    "content/shaders/blur.frag.qsb"
    "content/shaders/colorize.frag.qsb"
    "content/shaders/genie.vert.qsb"
    "content/shaders/outline.frag.qsb"
    "content/shaders/shadow.frag.qsb"
    "content/shaders/wobble.frag.qsb"
    "shadereffects.qml"
)

qt6_add_resources(shadereffects "shadereffects"
    PREFIX
        "/shadereffects"
    FILES
        ${shadereffects_resource_files}
)

install(TARGETS shadereffects
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
