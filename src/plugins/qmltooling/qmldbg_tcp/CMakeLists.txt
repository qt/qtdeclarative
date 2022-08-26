# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmldbg_tcp.pro.

#####################################################################
## QTcpServerConnectionFactory Plugin:
#####################################################################

qt_internal_add_plugin(QTcpServerConnectionFactoryPlugin
    OUTPUT_NAME qmldbg_tcp
    CLASS_NAME QTcpServerConnectionFactory
    PLUGIN_TYPE qmltooling
    SOURCES
        qtcpserverconnection.cpp
    LIBRARIES
        Qt::Network
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:qmldbg_tcp.pro:<TRUE>:
# OTHER_FILES = "$$PWD/qtcpserverconnection.json"
