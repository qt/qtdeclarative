# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(testbed LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/multieffect/testbed")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory("../../shared" "shared")

qt_add_executable(testbedexample WIN32 MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(testbedexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

add_dependencies(testbedexample testbed_shared)

# Resources:
qt_add_qml_module(testbedexample
    URI testbed
    QML_FILES
    "qml/FpsItem.qml"
    "qml/main.qml"
    "qml/ResetSettingsOverlay.qml"
    "qml/Settings.qml"
    "qml/SettingsView.qml"
    "qml/ShaderView.qml"
    "qml/TestMaskItem.qml"
    "qml/TestSourceItem.qml"
    "qml/WarningsItem.qml"
    "qml/WarningsView.qml"
    "qml/SettingsComponentView.qml"
    "qml/SettingsComponentSlider.qml"
    "qml/SettingsComponentCheckBox.qml"
    "qml/SettingsComponentColorSelector.qml"
    "qml/CustomMultiEffect/BlurHelper.qml"
    "qml/CustomMultiEffect/CustomMultiEffect.qml"
    RESOURCES
    "qml/images/pause.png"
    "qml/images/play.png"
    "qml/images/spinner.png"
    "qml/images/warning.png"
    "qml/images/arrow.png"
    "qml/images/Built_with_Qt.png"
    "qml/images/Built_with_Qt_RGB_logo.png"
    "qml/CustomMultiEffect/bluritems.frag.qsb"
    "qml/CustomMultiEffect/bluritems.vert.qsb"
    "qml/CustomMultiEffect/custommultieffect.frag.qsb"
    "qml/CustomMultiEffect/custommultieffect.vert.qsb"
)

install(TARGETS testbedexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(testbedexample)
