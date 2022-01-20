cmake_minimum_required(VERSION 3.16)
project(rendercontrol_opengl LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/rendercontrol/rendercontrol_opengl")

find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL Qml Quick)

qt_add_executable(rendercontrol_openglexample WIN32 MACOSX_BUNDLE
    cuberenderer.cpp
    main.cpp
    window_singlethreaded.cpp
)

target_link_libraries(rendercontrol_openglexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::OpenGL
    Qt::Qml
    Qt::Quick
)

# Resources:
qt_add_qml_module(rendercontrol_openglexample
    URI rendercontrol
    VERSION 1.0
    QML_FILES
        "demo.qml"
)

install(TARGETS rendercontrol_openglexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
