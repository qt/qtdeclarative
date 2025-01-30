# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.22)
    project(tst_qtquickview_signallistener LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
    find_package(Qt6 REQUIRED COMPONENTS Quick)
endif()

qt_standard_project_setup()

# Dummy test - just so this can be detected by ctest.
# Actual test implementation is under ${CMAKE_CURRENT_SOURCE_DIR}/android/qml/
# Add a source file, otherwise no test target is actually created.
qt_internal_add_test(tst_qtquickview_signallistener
    SOURCES
        empty.cpp
)

set_target_properties(tst_qtquickview_signallistener
    PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
)
