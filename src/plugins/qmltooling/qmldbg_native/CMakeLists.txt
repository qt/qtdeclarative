# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmldbg_native.pro.

#####################################################################
## QQmlNativeDebugConnectorFactory Plugin:
#####################################################################

qt_internal_add_plugin(QQmlNativeDebugConnectorFactoryPlugin
    OUTPUT_NAME qmldbg_native
    CLASS_NAME QQmlNativeDebugConnectorFactory
    PLUGIN_TYPE qmltooling
    SOURCES
        qqmlnativedebugconnector.cpp qqmlnativedebugconnector.h
    LIBRARIES
        Qt::CorePrivate
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:qmldbg_native.pro:<TRUE>:
# OTHER_FILES = "$$PWD/qqmlnativedebugconnector.json"
