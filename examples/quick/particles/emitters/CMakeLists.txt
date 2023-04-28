# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(emitters LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/particles/emitters")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory("../../shared" "shared")

qt_add_executable(emittersexample WIN32 MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(emittersexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

add_dependencies(emittersexample emitters_shared)

# Resources:
qt_add_qml_module(emittersexample
    URI emitters
    QML_FILES
        "burstandpulse.qml"
        "customemitter.qml"
        "emitmask.qml"
        "maximumemitted.qml"
        "shapeanddirection.qml"
        "trailemitter.qml"
        "velocityfrommotion.qml"
        "emitters.qml"
    RESOURCES
        "images/portal_bg.png"
        "images/starfish_mask.png"
)

install(TARGETS emittersexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(emittersexample)
