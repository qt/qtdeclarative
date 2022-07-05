# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_internal_add_plugin(QmlLintQuickPlugin
    OUTPUT_NAME quicklintplugin
    PLUGIN_TYPE qmllint
    SOURCES
        quicklintplugin.h
        quicklintplugin.cpp
    LIBRARIES
        Qt::QmlCompilerPrivate
)
