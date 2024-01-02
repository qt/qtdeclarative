# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(painteditem LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/customitems/painteditem")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory(TextBalloon)

qt_add_executable(painteditemexample WIN32 MACOSX_BUNDLE main.cpp)

qt_add_qml_module(painteditemexample
    URI painteditem
    QML_FILES
        "textballoons.qml"
)

target_link_libraries(painteditemexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

add_dependencies(painteditemexample qmltextballoon)

install(TARGETS painteditemexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

include(../../shared/QtBundleQmlModuleForMacOS.cmake)

set(app_target "painteditemexample")
set(qml_module "qmltextballoon")
add_qml_module_to_macos_app_bundle("${app_target}" "${qml_module}")
