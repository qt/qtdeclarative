# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(wearableexample VERSION 1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols/wearable")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2)

qt_standard_project_setup()

add_subdirectory(Wearable)
add_subdirectory(WearableSettings)
add_subdirectory(WearableStyle)

qt_add_executable(wearableexample WIN32 MACOSX_BUNDLE
    wearable.cpp
)

target_link_libraries(wearableexample PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
)

if(UNIX AND NOT APPLE AND CMAKE_CROSSCOMPILING)
    find_package(Qt6 REQUIRED COMPONENTS QuickTemplates2)

    # Work around QTBUG-86533
    target_link_libraries(wearableexample PRIVATE Qt6::QuickTemplates2)
endif()

set_target_properties(wearableexample PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.qt.examples.wearable"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

install(TARGETS wearableexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

include(../../quick/shared/QtBundleQmlModuleForMacOS.cmake)

set(app_target "wearableexample")
add_qml_module_to_macos_app_bundle("${app_target}" "wearable" "Wearable")
add_qml_module_to_macos_app_bundle("${app_target}" "wearablesettings" "WearableSettings")
add_qml_module_to_macos_app_bundle("${app_target}" "wearablestyle" "WearableStyle")
