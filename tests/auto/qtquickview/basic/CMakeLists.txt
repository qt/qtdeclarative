# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#[[
    In order to test QtQuickViews, we need to implement a custom Activity,
    which loads the QtQuickView, which in turn will load the Qt tests.
    So that it is still runnable via ctest, we use a generic Qt for Android
    project layout.

    This top-level project is an empty project with the
    QT_ANDROID_PACKAGE_SOURCE_DIR set, so that we can implement our own Android
    application code including a custom Activity, AndroidManifest, etc.

    The project gets built as usual, but the customized inner Android portions
    will use the Qt Gradle Plugin to build the QML project under android/qml/,
    which also contains the tests.
]]

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.22)
    project(tst_qtquickview_basic LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
    find_package(Qt6 REQUIRED COMPONENTS Quick)
endif()

qt_standard_project_setup()

# Dummy test - just so this can be detected by ctest.
# Actual test implementation is under ${CMAKE_CURRENT_SOURCE_DIR}/android/qml/
# Add a source file, otherwise no test target is actually created.
qt_internal_add_test(tst_qtquickview_basic
    SOURCES
        empty.cpp
)

set_target_properties(tst_qtquickview_basic
    PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
)
