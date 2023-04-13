# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_how-to-cpp-button
    SOURCES
        tst_how-to-cpp-button.cpp
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Quick
        Qt::QuickPrivate
        Qt::QuickControlsTestUtilsPrivate
        Qt::QuickTemplates2Private
)

qt_policy(SET QTP0001 NEW)

qt_add_qml_module(tst_how-to-cpp-button
    URI MyModule
    QML_FILES
        Main.qml
    SOURCES
        backend.h
        backend.cpp
)
