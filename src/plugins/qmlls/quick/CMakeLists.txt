# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_plugin(QmlLSQuickPlugin
    CLASS_NAME QQmlLSQuickPlugin
    OUTPUT_NAME qmllsquickplugin
    PLUGIN_TYPE qmlls
    SOURCES
        qqmllsquickplugin_p.h
        qqmllsquickplugin.cpp
    LIBRARIES
        Qt::QmlCompilerPrivate
        Qt::QmlLSPrivate
)
