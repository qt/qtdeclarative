# Generated from shadereffects.pro.

cmake_minimum_required(VERSION 3.14)
project(shadereffects LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)

add_qt_gui_executable(shadereffects
    main.cpp
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
    "content/shaders/+hlsl/blur.frag"
    "content/shaders/+hlsl/colorize.frag"
    "content/shaders/+hlsl/genie.vert"
    "content/shaders/+hlsl/outline.frag"
    "content/shaders/+hlsl/shadow.frag"
    "content/shaders/+hlsl/wobble.frag"
    "content/shaders/+qsb/blur.frag"
    "content/shaders/+qsb/colorize.frag"
    "content/shaders/+qsb/genie.vert"
    "content/shaders/+qsb/outline.frag"
    "content/shaders/+qsb/shadow.frag"
    "content/shaders/+qsb/wobble.frag"
    "content/shaders/blur.frag"
    "content/shaders/colorize.frag"
    "content/shaders/genie.vert"
    "content/shaders/outline.frag"
    "content/shaders/shadow.frag"
    "content/shaders/wobble.frag"
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
