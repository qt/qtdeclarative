# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from nokeywords.pro.

#####################################################################
## tst_nokeywords Test:
#####################################################################

qt_internal_add_test(tst_nokeywords
    SOURCES
        tst_nokeywords.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::Quick
        Qt::QuickPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_nokeywords CONDITION QT_FEATURE_opengl
    LIBRARIES
        Qt::OpenGLPrivate
)
