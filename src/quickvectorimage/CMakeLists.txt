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
    GENERATE_CPP_EXPORTS
)
