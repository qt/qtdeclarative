# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_qml_module(QmlXmlListModel
    URI "QtQml.XmlListModel"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_TARGET qmlxmllistmodelplugin
    CLASS_NAME QtQmlXmlListModelPlugin
    DEPENDENCIES
        QtQml/auto
    SOURCES
        qqmlxmllistmodel_p.h qqmlxmllistmodel.cpp
        qtqmlxmllistmodelglobal_p.h
    DEFINES
        QT_BUILD_QMLXMLLISTMODEL_LIB
    LIBRARIES
        Qt::CorePrivate
        Qt::QmlModelsPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)

qt_internal_add_docs(QmlXmlListModel
    doc/qtqmlxmllistmodel.qdocconf
)
