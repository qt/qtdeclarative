# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmldbg_local.pro.

#####################################################################
## QLocalClientConnectionFactory Plugin:
#####################################################################

qt_internal_add_plugin(QLocalClientConnectionFactoryPlugin
    OUTPUT_NAME qmldbg_local
    CLASS_NAME QLocalClientConnectionFactory
    PLUGIN_TYPE qmltooling
    SOURCES
        qlocalclientconnection.cpp
    LIBRARIES
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:qmldbg_local.pro:<TRUE>:
# OTHER_FILES = "$$PWD/qlocalclientconnection.json"
