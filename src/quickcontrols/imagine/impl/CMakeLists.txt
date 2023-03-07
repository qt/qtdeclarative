# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtquickcontrols2imaginestyleimplplugin Plugin:
#####################################################################

set(qml_files
    "OpacityMask.qml"
)

qt_internal_add_qml_module(qtquickcontrols2imaginestyleimplplugin
    URI "QtQuick.Controls.Imagine.impl"
    VERSION "${PROJECT_VERSION}"
    CLASS_NAME QtQuickControls2ImagineStyleImplPlugin
    PLUGIN_TARGET qtquickcontrols2imaginestyleimplplugin
    NO_PLUGIN_OPTIONAL
    QML_FILES
        ${qml_files}
    IMPORTS
        QtQuick.Controls.impl/auto
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::QmlPrivate
        Qt::QuickControls2ImplPrivate
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
)

qt_internal_add_shaders(qtquickcontrols2imaginestyleimplplugin "qtquickcontrols2imaginestyleimplplugin_shaders"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/qt-project.org/imports/QtQuick/Controls/Imagine/impl"
    FILES
        "shaders/OpacityMask.frag"
)
