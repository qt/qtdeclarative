# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.22)

project(tst_qtquickview_signallistener_qml LANGUAGES CXX)

find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
find_package(Qt6 REQUIRED COMPONENTS Quick QuickTest QuickTestPrivate)

qt_standard_project_setup()

qt_add_executable(tst_qtquickview_signallistener_qml
    tst_qtquickview.cpp)

set_source_files_properties(TestView.qml PROPERTIES QT_QML_GENERATE_JAVA_CLASS TRUE)
qt_add_qml_module(tst_qtquickview_signallistener_qml
    URI TestViewModule
    VERSION 1.0
    QML_FILES
        TestView.qml
    SOURCES
        testactivitycommunicator.cpp testactivitycommunicator.h
)

set_target_properties(tst_qtquickview_signallistener_qml
    PROPERTIES
        QT_ANDROID_PACKAGE_NAME "org.qtproject.qt.android.qtquickview_signallistener_qml")

target_link_libraries(tst_qtquickview_signallistener_qml PRIVATE
    Qt::Core Qt::Quick Qt::QuickTest Qt::QuickTestPrivate
)
