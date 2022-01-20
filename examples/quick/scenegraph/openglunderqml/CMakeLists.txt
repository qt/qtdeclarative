cmake_minimum_required(VERSION 3.16)
project(openglunderqml LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/scenegraph/openglunderqml")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_executable(openglunderqml
    main.cpp
    squircle.cpp squircle.h
)

set_target_properties(openglunderqml PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE

    # Prevent name clash with build subdirectory on case-insensitive file systems
    OUTPUT_NAME openglunderqmlapp
)

target_link_libraries(openglunderqml PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

qt_add_qml_module(openglunderqml
    URI OpenGLUnderQML
    VERSION 1.0
    QML_FILES main.qml
    RESOURCE_PREFIX /scenegraph/openglunderqml
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS openglunderqml
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
