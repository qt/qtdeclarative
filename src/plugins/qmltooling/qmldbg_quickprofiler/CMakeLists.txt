# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmldbg_quickprofiler.pro.

#####################################################################
## QQuickProfilerAdapterFactory Plugin:
#####################################################################

qt_internal_add_plugin(QQuickProfilerAdapterFactoryPlugin
    OUTPUT_NAME qmldbg_quickprofiler
    CLASS_NAME QQuickProfilerAdapterFactory
    PLUGIN_TYPE qmltooling
    SOURCES
        qquickprofileradapter.cpp qquickprofileradapter.h
        qquickprofileradapterfactory.cpp qquickprofileradapterfactory.h
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
)

#### Keys ignored in scope 1:.:.:qmldbg_quickprofiler.pro:<TRUE>:
# OTHER_FILES = "qquickprofileradapter.json"
