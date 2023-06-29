# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_how-to-cpp-enum-js LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_how-to-cpp-enum-js
    SOURCES
        backend.cpp
        backend.h
        tst_how-to-cpp-enum-js.cpp
    LIBRARIES
        Qt::Core
        Qt::Qml
)

qt_add_resources(tst_how-to-cpp-enum-js "js"
    PREFIX
        /
    FILES
        script.mjs
)
