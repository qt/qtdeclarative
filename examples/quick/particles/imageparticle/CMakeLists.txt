# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(imageparticle LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory("../../shared" "shared")

qt_add_executable(imageparticleexample WIN32 MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(imageparticleexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

add_dependencies(imageparticleexample imageparticle_shared)

# Resources:
qt_add_qml_module(imageparticleexample
    URI imageparticle
    QML_FILES
        "allatonce.qml"
        "colored.qml"
        "colortable.qml"
        "deformation.qml"
        "imageparticle.qml"
        "rotation.qml"
        "sharing.qml"
        "sprites.qml"
    RESOURCES
        "images/bear_tiles.png"
        "images/colortable.png"
        "images/starfish_0.png"
        "images/starfish_1.png"
        "images/starfish_2.png"
        "images/starfish_3.png"
        "images/starfish_4.png"
        "images/flower.png"
)

install(TARGETS imageparticleexample
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET imageparticleexample
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
