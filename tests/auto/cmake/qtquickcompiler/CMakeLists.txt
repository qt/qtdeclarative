# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.16)
project(qqc_test)

find_package(Qt6 ${CMAKE_Core_MODULE_MAJOR_VERSION}.${CMAKE_Core_MODULE_MINOR_VERSION}
    REQUIRED COMPONENTS Qml Gui Test
)

set(CMAKE_AUTOMOC ON)

qt_add_executable(qqc_test
    # Absolute path needed to avoid using the copied file in the build dir.
    # That would still work, but it should use the file in the source dir.
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)
target_link_libraries(qqc_test PRIVATE Qt6::Gui Qt6::Qml Qt6::Test)

set_source_files_properties("resources with space/main.qml" PROPERTIES
    QT_RESOURCE_ALIAS main.qml
)

qt_add_qml_module(qqc_test
    URI Test
    VERSION 1.0
    QML_FILES
        "resources with space/main.qml"
    RESOURCES
        main.cpp
    RESOURCE_PREFIX /
)
