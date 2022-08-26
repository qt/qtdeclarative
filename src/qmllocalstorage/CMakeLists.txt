# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_qml_module(QmlLocalStorage
    URI "QtQuick.LocalStorage"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_TARGET qmllocalstorageplugin
    CLASS_NAME QQmlLocalStoragePlugin
    SOURCES
        qqmllocalstorage.cpp qqmllocalstorage_p.h
        qqmllocalstorageglobal_p.h
    DEFINES
        QT_BUILD_QMLLOCALSTORAGE_LIB
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
        Qt::Sql
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)
