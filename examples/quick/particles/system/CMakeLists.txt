# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(system LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(systemexample WIN32 MACOSX_BUNDLE
    main.cpp
)

set_target_properties(systemexample PROPERTIES FOLDER "qtc_runnable")

target_link_libraries(systemexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

# Resources:
qt_add_qml_module(systemexample
    URI system
    QML_FILES
        "LauncherList.qml"
        "SimpleLauncherDelegate.qml"
        "dynamiccomparison.qml"
        "dynamicemitters.qml"
        "multiplepainters.qml"
        "timedgroupchanges.qml"
        "startstop.qml"
        "system.qml"
    RESOURCES
        "images/back.png"
        "images/back@2x.png"
        "images/back@3x.png"
        "images/back@4x.png"
        "images/sparkleSize.png"
        "images/particle2.png"
        "images/particle3.png"
)

install(TARGETS systemexample
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET systemexample
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
