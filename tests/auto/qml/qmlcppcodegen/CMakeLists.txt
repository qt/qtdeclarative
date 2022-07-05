# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

add_subdirectory(data)

qt_internal_add_test(tst_qmlcppcodegen
    SOURCES
        tst_qmlcppcodegen.cpp
    LIBRARIES
        Qt::QmlPrivate
        Qt::Gui
        codegen_test_module
        codegen_test_moduleplugin
)
