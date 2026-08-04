# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_qml_module(QmlDesignSupport
    URI "QtQml.DesignSupport"
    VERSION "${PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    DEPENDENCIES
        QtQml/auto
    SOURCES
        qabstractobjectregistryref.cpp
        qabstractobjectregistryref.h
        qabstractobjectregistryref_p.h
        qmultiobjectregistryref.cpp
        qmultiobjectregistryref.h
        qmultiobjectregistryref_p.h
        qobjectregistry.cpp
        qobjectregistry_p.h
        qobjectregistryref.cpp
        qobjectregistryref.h
        qobjectregistryref_p.h
        qobjectregistrysingleton.cpp
        qobjectregistrysingleton_p.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
    LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::QmlPrivate
)

qt_internal_add_docs(QmlDesignSupport
    doc/qtqmldesignsupport.qdocconf
)

target_sources(QmlDesignSupport
    PRIVATE
        doc/src/qtqmldesignsupport.qdoc
)
