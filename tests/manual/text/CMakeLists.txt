# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(text_manual_test LANGUAGES CXX)

# Skip building this test in a top-level build configured with -make tests
# to avoid configuration errors like
#  CMake Error: AUTOMOC for target tst_template_full_path: The "moc"
#  executable ".../qtbase/libexec/moc" does not exist.
#
# It happens because targets are added using non-internal API which do not call
# qt_autogen_tools_initial_setup.
if(QT_SUPERBUILD OR QT_BUILDING_QT)
    return()
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Quick QuickControls2)

qt_add_executable(text_manual_test
    main.cpp
)
target_link_libraries(text_manual_test PUBLIC
        Qt::Gui
        Qt::Quick
        Qt::QuickControls2
)

set(qmake_immediate_resource_files
    "Rule.qml"
    "SignalIndicator.qml"
    "textInputPropertiesAndSignals.qml"
)

qt6_add_resources(text_manual_test "qml"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)
