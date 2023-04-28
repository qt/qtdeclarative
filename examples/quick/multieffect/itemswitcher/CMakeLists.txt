# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(itemswitcher LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/multieffect/itemswitcher")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory("../../shared" "shared")

qt_add_executable(itemswitcherexample WIN32 MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(itemswitcherexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

add_dependencies(itemswitcherexample itemswitcher_shared)

# Resources:
qt_add_qml_module(itemswitcherexample
    URI itemswitcher
    QML_FILES
    "qml/main.qml"
    "qml/PagesView.qml"
    "qml/SwitchEffect3DFlip.qml"
    "qml/SwitchEffectBlur.qml"
    "qml/SwitchEffectStars.qml"
    "qml/ItemSwitcher.qml"
    "qml/PagesItem.qml"
    "qml/SettingsView.qml"
    "qml/SwitchEffectBlinds.qml"
    "qml/SwitchEffectHeart.qml"
    "qml/SwitchEffectThunder.qml"
    "qml/SettingsComponentButton.qml"
    "qml/SettingsComponentSlider.qml"
    RESOURCES
    "qml/images/background.png"
    "qml/images/hblinds.png"
    "qml/images/heart.png"
    "qml/images/quit_coding.png"
    "qml/images/smoke.png"
    "qml/images/star.png"
    "qml/images/stripes.png"
    "qml/images/arrow.png"
    "qml/images/Built_with_Qt.png"
    "qml/images/Built_with_Qt_RGB_logo.png"
)

install(TARGETS itemswitcherexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(itemswitcherexample)
