# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.21)
project(qis_test)

find_package(Qt6 REQUIRED COMPONENTS Core Test Qml)

set(CMAKE_AUTOMOC ON)

# We explicitly don't want to use qt_add_executable() here because we want to
# manually invoke qt6_import_qml_plugins() ourselves later
add_executable(qis_test main.cpp)
target_link_libraries(qis_test PRIVATE Qt6::Test Qt6::Qml Qt6::Core)

qt6_add_qml_module(qis_test
    URI Test
    VERSION 1.0
    NO_RESOURCE_TARGET_PATH
    QML_FILES main.qml
)

qt6_import_qml_plugins(qis_test)
