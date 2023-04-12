# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QuickFigmaStyle Module:
#####################################################################

qt_internal_add_qml_module(QuickFigmaStyle
    URI "QtQuick.FigmaStyle"
    VERSION "${PROJECT_VERSION}"
    CLASS_NAME QtQuickFigmaStylePlugin
    PLUGIN_TARGET qtquickfigmastyleplugin
    SOURCES
        qquickfigmaconfigreader_p.h qquickfigmaconfigreader.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
        Qt::QuickControls2ImplPrivate
        Qt::QuickControls2Private
        Qt::QuickPrivate
)

