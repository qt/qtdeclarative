# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_qml_module(QmlNetwork
    URI "QtNetwork"
    VERSION "${PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    DEPENDENCIES
        QtQml/auto
    SOURCES
        qqmlnetworkinformation_p.h
        qqmlnetworkinformation.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Network
        Qt::Qml
    )

qt_internal_extend_target(QmlNetwork CONDITION QT_FEATURE_qml_ssl
    SOURCES
        ssl/qqmlsslnamespace_p.h ssl/qqmlsslsocketnamespace_p.h
        ssl/qqmlsslconfiguration.cpp ssl/qqmlsslconfiguration_p.h
        ssl/qqmlsslkey_p.h ssl/qqmlsslkey.cpp
)

qt_internal_add_docs(QmlNetwork
    doc/qtqmlnetwork.qdocconf
)
