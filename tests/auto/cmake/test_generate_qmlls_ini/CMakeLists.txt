# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.19)
project(tst_generate_qmlls_ini)

find_package(Qt6 REQUIRED COMPONENTS Core Qml Test)

qt_standard_project_setup()

qt_add_executable(tst_generate_qmlls_ini main.cpp)
target_link_libraries(tst_generate_qmlls_ini PRIVATE Qt6::Test)

set(QT_QML_GENERATE_QMLLS_INI ON CACHE BOOL "" FORCE)

add_subdirectory(SomeSubfolder)
add_subdirectory(Dotted)

qt_add_qml_module(tst_generate_qmlls_ini
    URI MainModule
    VERSION 1.0
    NO_RESOURCE_TARGET_PATH
    SOURCES
        main.cpp
    QML_FILES
        Main.qml
)
target_compile_definitions(tst_generate_qmlls_ini
    PRIVATE
        "SOURCE_DIRECTORY=u\"${CMAKE_CURRENT_SOURCE_DIR}\"_s"
        "BUILD_DIRECTORY=u\"${CMAKE_CURRENT_BINARY_DIR}\"_s"
)

qt_add_qml_module(Module
    URI Module
    VERSION 1.0
    QML_FILES
        Main.qml
    OUTPUT_DIRECTORY ./qml/hello/subfolders/Module
)

# Ensure linting runs when building the default "all" target
set_target_properties(all_qmllint PROPERTIES EXCLUDE_FROM_ALL FALSE)
