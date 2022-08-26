# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmldbg_inspector.pro.

#####################################################################
## QQmlInspectorServiceFactory Plugin:
#####################################################################

qt_internal_add_plugin(QQmlInspectorServiceFactoryPlugin
    OUTPUT_NAME qmldbg_inspector
    CLASS_NAME QQmlInspectorServiceFactory
    PLUGIN_TYPE qmltooling
    SOURCES
        globalinspector.cpp globalinspector.h
        highlight.cpp highlight.h
        inspecttool.cpp inspecttool.h
        qqmlinspectorservice.cpp
        qquickwindowinspector.cpp qquickwindowinspector.h
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
)

#### Keys ignored in scope 1:.:.:qmldbg_inspector.pro:<TRUE>:
# OTHER_FILES = "qqmlinspectorservice.json"
