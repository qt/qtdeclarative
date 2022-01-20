cmake_minimum_required(VERSION 3.16)
project(shadereffects LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/shadereffects")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick ShaderTools)

qt_add_executable(shadereffectsexample
    main.cpp
)

set_target_properties(shadereffectsexample PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(shadereffectsexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

qt_add_qml_module(shadereffectsexample
    URI shadereffects
    VERSION 1.0
    QML_FILES
        "shadereffects.qml"
    RESOURCES
        "content/face-smile.png"
        "content/qt-logo.png"
)

qt6_add_shaders(shadereffectsexample "shaders"
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/shadereffects"
    FILES
        "content/shaders/genie.vert"
        "content/shaders/blur.frag"
        "content/shaders/colorize.frag"
        "content/shaders/outline.frag"
        "content/shaders/shadow.frag"
        "content/shaders/wobble.frag"
)

install(TARGETS shadereffectsexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
