cmake_minimum_required(VERSION 3.16)
project(canvas LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/canvas")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

add_subdirectory("../shared" "shared")

qt_add_executable(canvasexample
    main.cpp
)

set_target_properties(canvasexample PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(canvasexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

qt_add_qml_module(canvasexample
    URI canvas
    VERSION 1.0
    QML_FILES
        "LabeledSlider.qml"
        "bezierCurve/bezierCurve.qml"
        "canvas.qml"
        "clip/clip.qml"
        "quadraticCurveTo/quadraticCurveTo.qml"
        "roundedrect/roundedrect.qml"
        "smile/smile.qml"
        "squircle/squircle.qml"
        "tiger/tiger.js"
        "tiger/tiger.qml"
    RESOURCES
        "contents/qt-logo.png"
        "squircle/squircle.png"
)

install(TARGETS canvasexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(canvasexample)
