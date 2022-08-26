# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_qml_module(LabsWavefrontMesh
    URI "Qt.labs.wavefrontmesh"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_TARGET qmlwavefrontmeshplugin
    CLASS_NAME QmlWavefrontMeshPlugin
    DEPENDENCIES
        QtQuick/auto
    SOURCES
        qwavefrontmesh.cpp qwavefrontmesh_p.h
        qqmlwavefrontmeshglobal_p.h
    DEFINES
        QT_BUILD_LABSWAVEFRONTMESH_LIB
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QuickPrivate
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)
