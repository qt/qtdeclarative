# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(system LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/particles/system")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory("../../shared" "shared")

qt_add_executable(systemexample WIN32 MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(systemexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

add_dependencies(systemexample system_shared)

# Resources:
qt_add_qml_module(systemexample
    URI system
    QML_FILES
        "dynamiccomparison.qml"
        "dynamicemitters.qml"
        "multiplepainters.qml"
        "timedgroupchanges.qml"
        "startstop.qml"
        "system.qml"
    RESOURCES
        "images/sparkleSize.png"
        "images/particle2.png"
        "images/particle3.png"
)

install(TARGETS systemexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

bundle_shared(systemexample)
