# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmlextensionplugin.pro.

#####################################################################
## tst_qqmlextensionplugin Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qqmlextensionplugin LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qqmlextensionplugin
    SOURCES
        tst_qqmlextensionplugin.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
)

# On Android we need to have some QML file, so that plugins are actually copied
# into the APK
if(ANDROID)
    add_compile_definitions(ANDROID_ARCH="${CMAKE_ANDROID_ARCH_ABI}")
    qt_add_resources(tst_qqmlextensionplugin "dummy_resources"
        PREFIX
            "/"
        FILES
            "./data/dummy.qml"
    )
endif()

## Scopes:
#####################################################################
