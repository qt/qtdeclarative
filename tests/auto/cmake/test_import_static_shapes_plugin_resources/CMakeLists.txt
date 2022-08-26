# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(test_project LANGUAGES CXX)
enable_testing()

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 COMPONENTS REQUIRED Quick Test QuickTestUtilsPrivate)

function(create_test_executable target)
    qt_add_executable(${target}
        MANUAL_FINALIZATION
        tst_main.cpp
    )
    target_link_libraries(${target} PRIVATE
        Qt::Qml
        Qt::Quick
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
        Qt::Test
    )

    set(app_resource_files
        "app.qml"
    )

    qt6_add_resources(${target} "app"
        PREFIX
            "/"
        FILES
            ${app_resource_files}
    )

    # Explicitly import qml plugins, regardless of lower CMake versions.
    qt_import_qml_plugins(${target})

    # Explicitly call finalizers, regardless of lower CMake versions.
    qt_finalize_target(${target})

    # Rune the app to ensure resources were linked and found, so there should be no qml errors.
    add_test(test_${target} ${target})
endfunction()

create_test_executable(app)
