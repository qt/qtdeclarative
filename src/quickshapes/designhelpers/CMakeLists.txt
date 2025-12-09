# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_qml_module(QuickShapesDesignHelpersPrivate
    URI "QtQuick.Shapes.DesignHelpers"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_TARGET qtquickshapesdesignhelpersplugin
    CLASS_NAME QtQuickShapesDesignHelpersPlugin
    DEPENDENCIES
        QtQuick/auto
    INTERNAL_MODULE
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
        qquickshapesdesignhelpersglobal_p.h
        qquickstarshape.cpp
        qquickstarshape_p.h
        qquickstarshape_p_p.h
    NO_UNITY_BUILD_SOURCES
        qquickstarshape.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::GuiPrivate
        Qt::Qml
        Qt::QuickPrivate
        Qt::QuickShapesPrivate
)
