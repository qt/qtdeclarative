# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qquickwindow.pro.

#####################################################################
## tst_qquickwindow Binary:
#####################################################################

qt_internal_add_benchmark(tst_qquickwindow_bench # special case
    SOURCES
        tst_qquickwindow.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::OpenGLPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qquickwindow.pro:<TRUE>:
# DEPLOYMENT = "testDataFiles"
# testDataFiles.files = "data"
# testDataFiles.path = "."

## Scopes:
#####################################################################
