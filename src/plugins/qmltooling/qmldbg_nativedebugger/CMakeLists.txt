# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmldbg_nativedebugger.pro.

#####################################################################
## QQmlNativeDebugServiceFactory Plugin:
#####################################################################

qt_internal_add_plugin(QQmlNativeDebugServiceFactoryPlugin
    OUTPUT_NAME qmldbg_nativedebugger
    CLASS_NAME QQmlNativeDebugServiceFactory
    PLUGIN_TYPE qmltooling
    SOURCES
        qqmlnativedebugservice.cpp qqmlnativedebugservice.h
        qqmlnativedebugservicefactory.cpp qqmlnativedebugservicefactory.h
    LIBRARIES
        Qt::Core
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:qmldbg_nativedebugger.pro:<TRUE>:
# OTHER_FILES = "$$PWD/qqmlnativedebugservice.json"
