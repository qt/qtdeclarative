# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QuickVectorGraphics Module:
#####################################################################

qt_internal_add_module(QuickVectorImageGeneratorPrivate
    INTERNAL_MODULE
    SOURCES
        generator/qsvgvisitorimpl_p.h generator/qsvgvisitorimpl.cpp
        generator/qquickgenerator_p.h generator/qquickgenerator.cpp
        generator/qquickitemgenerator_p.h generator/qquickitemgenerator.cpp
        generator/qquickqmlgenerator_p.h generator/qquickqmlgenerator.cpp
        generator/qquicknodeinfo_p.h
        generator/utils_p.h
        qquickvectorimageglobal_p.h
    LIBRARIES
        Qt::Core
        Qt::QuickPrivate
        Qt::QuickShapesPrivate
        Qt::SvgPrivate
)

qt_internal_add_qml_module(QuickVectorImage
    URI "QtQuick.VectorImage"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_TARGET qquickvectorimageplugin
    NO_PLUGIN_OPTIONAL
    CLASS_NAME QtQuickVectorImagePlugin
    SOURCES
    qquickvectorimage_p.h qquickvectorimage.cpp
    qquickvectorimage_p_p.h
    LIBRARIES
        Qt::QuickPrivate
        Qt::QuickVectorImageGeneratorPrivate
        Qt::SvgPrivate
)
