cmake_minimum_required(VERSION 3.16)
project(rendercontrol_d3d11 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/rendercontrol/rendercontrol_d3d11")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_executable(rendercontrol_d3d11example WIN32 MACOSX_BUNDLE
    engine.cpp
    main.cpp
    window.cpp
)

target_link_libraries(rendercontrol_d3d11example PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
    d3d11
    dxgi
    dxguid
    uuid
)

# Resources:
qt_add_qml_module(rendercontrol_d3d11example
    URI rendercontrol
    VERSION 1.0
    QML_FILES
        "demo.qml"
)

install(TARGETS rendercontrol_d3d11example
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
