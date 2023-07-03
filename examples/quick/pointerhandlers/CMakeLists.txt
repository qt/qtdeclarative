# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(pointerhandlers LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/pointerhandlers")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Svg)

qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory("../shared" "shared")

qt_add_executable(pointerhandlersexample WIN32 MACOSX_BUNDLE main.cpp)

target_link_libraries(pointerhandlersexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Svg
)

add_dependencies(pointerhandlersexample pointerhandlers_shared)

qt_add_qml_module(pointerhandlersexample
    URI pointerhandlers
    QML_FILES
        "components/Button.qml"
        "components/CheckBox.qml"
        "components/CorkPanel.qml"
        "components/FakeFlickable.qml"
        "components/FlashAnimation.qml"
        "components/LeftDrawer.qml"
        "components/MomentumAnimation.qml"
        "components/MouseFeedbackSprite.qml"
        "components/QuadPieMenu.qml"
        "components/ScrollBar.qml"
        "components/Slider.qml"
        "components/TouchpointFeedbackSprite.qml"
        "corkboards.qml"
        "fakeFlickable.qml"
        "flingAnimation.qml"
        "joystick.qml"
        "map.qml"
        "mixer.qml"
        "multibuttons.qml"
        "multiflame.qml"
        "pieMenu.qml"
        "pinchHandler.qml"
        "pointerhandlers.qml"
        "sidebar.qml"
        "singlePointHandlerProperties.qml"
        "tabletCanvasDrawing.qml"
        "tapHandler.qml"
    RESOURCES
        "components/images/checkmark.png"
        "components/images/cork.jpg"
        "components/images/fingersprite.png"
        "components/images/mixer-knob.png"
        "components/images/mouse.png"
        "components/images/mouse_left.png"
        "components/images/mouse_middle.png"
        "components/images/mouse_right.png"
        "components/images/mouse_wheel_ridges.png"
        "components/images/note-yellow.png"
        "components/images/tack.png"
        "images/arrowhead.png"
        "images/balloon.png"
        "images/blur-circle.png"
        "images/cursor-airbrush.png"
        "images/cursor-eraser.png"
        "images/cursor-felt-marker.png"
        "images/cursor-pencil.png"
        "images/fighter.png"
        "images/grabbing-location.svg"
        "images/joystick-outer-case-pov.jpg"
        "images/map.svgz"
        "images/missile.png"
        "images/redball.png"
)

install(TARGETS pointerhandlersexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(pointerhandlersexample)
