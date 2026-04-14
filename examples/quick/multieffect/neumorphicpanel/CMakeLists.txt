# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(neumorphicpanel LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory("../../shared" "shared")

qt_add_executable(neumorphicpanelexample WIN32 MACOSX_BUNDLE
    main.cpp
)

set_target_properties(neumorphicpanelexample PROPERTIES FOLDER "qtc_runnable")

target_link_libraries(neumorphicpanelexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

add_dependencies(neumorphicpanelexample neumorphicpanel_shared)

# Resources:
qt_add_qml_module(neumorphicpanelexample
    URI neumorphicpanel
    QML_FILES
    "qml/main.qml"
    "qml/Settings.qml"
    "qml/SettingsView.qml"
    "qml/SettingsComponentSlider.qml"
    "qml/SettingsComponentButton.qml"
    "qml/SettingsComponentCheckBox.qml"
    "qml/CloudyRectMaterial/CloudyRectMaterial.qml"
    RESOURCES
    "qml/images/arrow.png"
    "qml/images/qt_logo_white_rgb.png"
    "qml/CloudyRectMaterial/cloudyrectmaterial.vert.qsb"
    "qml/CloudyRectMaterial/cloudyrectmaterial.frag.qsb"
)

install(TARGETS neumorphicpanelexample
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET neumorphicpanelexample
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
