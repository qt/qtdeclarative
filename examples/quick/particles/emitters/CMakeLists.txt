# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(emitters LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.8)

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
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET emittersexample
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
