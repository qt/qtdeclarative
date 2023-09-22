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
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)

qt_internal_add_docs(QmlNetwork
    doc/qtqmlnetwork.qdocconf
)