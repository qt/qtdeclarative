# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
