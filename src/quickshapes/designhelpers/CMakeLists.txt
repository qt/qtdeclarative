# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_qml_module(QuickShapesDesignHelpers
    URI "QtQuick.Shapes.DesignHelpers"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_TARGET qtquickshapesdesignhelpersplugin
    CLASS_NAME QtQuickShapesDesignHelpersPlugin
    DEPENDENCIES
        QtQuick/auto
        QtQuick.Shapes/auto
    SOURCES
        qquickellipseshape.cpp
        qquickellipseshape_p.h
        qquickellipseshape_p_p.h
        qquickrectangleshape.cpp
        qquickrectangleshape_p.h
        qquickrectangleshape_p_p.h
        qquickregularpolygonshape.cpp
        qquickregularpolygonshape_p.h
        qquickregularpolygonshape_p_p.h
        qquickshapesdesignhelpersglobal.h
        qquickshapesdesignhelpersglobal_p.h
        qquickstarshape.cpp
        qquickstarshape_p.h
        qquickstarshape_p_p.h
    NO_UNITY_BUILD_SOURCES
        qquickstarshape.cpp
    LIBRARIES
        Qt::GuiPrivate
        Qt::QuickPrivate
        Qt::QuickShapesPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
    PRIVATE_MODULE_INTERFACE
        Qt::GuiPrivate
        Qt::QuickPrivate
        Qt::QuickShapesPrivate
)

target_sources(QuickShapesDesignHelpers
    PRIVATE
        doc/src/qtquickshapesdesignhelpers.qdoc
)
