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
        generator/qquickanimatedproperty_p.h
        generator/qquicknodeinfo_p.h
        generator/utils_p.h
        qquickvectorimageglobal_p.h
        qquickvectorimageplugin_p.h qquickvectorimageplugin.cpp
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
    CLASS_NAME QtQuickVectorImagePlugin
    DEPENDENCIES
        QtQuick/auto
        QtQuick.Shapes/auto
        QtQuick.Effects/auto
    IMPORTS
        QtQuick.VectorImage.Helpers/auto
    SOURCES
    qquickvectorimage_p.h qquickvectorimage.cpp
    qquickvectorimageincubator_p.h qquickvectorimageincubator.cpp
    qquickvectorimage_p_p.h
    LIBRARIES
        Qt::QuickPrivate
        Qt::QuickVectorImageGeneratorPrivate
        Qt::SvgPrivate
)

qt_internal_add_qml_module(QuickVectorImageHelpers
    URI "QtQuick.VectorImage.Helpers"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_TARGET qquickvectorimagehelpersplugin
    NO_PLUGIN_OPTIONAL
    CLASS_NAME QtQuickVectorImageHelpersPlugin
    DEPENDENCIES
        QtQuick/auto
    SOURCES
        helpers/qquickcoloropacityanimation_p.h helpers/qquickcoloropacityanimation.cpp
        helpers/qquicktransformgroup_p.h helpers/qquicktransformgroup.cpp
        helpers/qquickitemspy_p.h helpers/qquickitemspy.cpp
        helpers/qquickpathinterpolated_p.h helpers/qquickpathinterpolated.cpp
    LIBRARIES
        Qt::QuickPrivate
        Qt::QuickVectorImageGeneratorPrivate
)

qt_internal_add_shaders(QuickVectorImageHelpers "vectorimage_shaders"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    MULTIVIEW
    PREFIX
        "/qt-project.org/quickvectorimage/"
    FILES
        "helpers/shaders_ng/genericmask.frag"
        "helpers/shaders_ng/fecolormatrix.frag"
        "helpers/shaders_ng/femerge.frag"
)

qt_internal_add_shaders(QuickVectorImageHelpers "vectorimage_shaders_comp_over"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    MULTIVIEW
    DEFINES
        "COMPOSITE_OVER"
    PREFIX
        "/qt-project.org/quickvectorimage/"
    FILES
        "helpers/shaders_ng/fecomposite.frag"
    OUTPUTS
        "helpers/shaders_ng/fecompositeover.frag.qsb"
)

qt_internal_add_shaders(QuickVectorImageHelpers "vectorimage_shaders_comp_in"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    MULTIVIEW
    DEFINES
        "COMPOSITE_IN"
    PREFIX
        "/qt-project.org/quickvectorimage/"
    FILES
        "helpers/shaders_ng/fecomposite.frag"
    OUTPUTS
        "helpers/shaders_ng/fecompositein.frag.qsb"
)

qt_internal_add_shaders(QuickVectorImageHelpers "vectorimage_shaders_comp_out"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    MULTIVIEW
    DEFINES
        "COMPOSITE_OUT"
    PREFIX
        "/qt-project.org/quickvectorimage/"
    FILES
        "helpers/shaders_ng/fecomposite.frag"
    OUTPUTS
        "helpers/shaders_ng/fecompositeout.frag.qsb"
)

qt_internal_add_shaders(QuickVectorImageHelpers "vectorimage_shaders_comp_atop"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    MULTIVIEW
    DEFINES
        "COMPOSITE_ATOP"
    PREFIX
        "/qt-project.org/quickvectorimage/"
    FILES
        "helpers/shaders_ng/fecomposite.frag"
    OUTPUTS
        "helpers/shaders_ng/fecompositeatop.frag.qsb"
)

qt_internal_add_shaders(QuickVectorImageHelpers "vectorimage_shaders_comp_xor"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    MULTIVIEW
    DEFINES
        "COMPOSITE_XOR"
    PREFIX
        "/qt-project.org/quickvectorimage/"
    FILES
        "helpers/shaders_ng/fecomposite.frag"
    OUTPUTS
        "helpers/shaders_ng/fecompositexor.frag.qsb"
)

qt_internal_add_shaders(QuickVectorImageHelpers "vectorimage_shaders_comp_lighter"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    MULTIVIEW
    DEFINES
        "COMPOSITE_LIGHTER"
    PREFIX
        "/qt-project.org/quickvectorimage/"
    FILES
        "helpers/shaders_ng/fecomposite.frag"
    OUTPUTS
        "helpers/shaders_ng/fecompositelighter.frag.qsb"
)

qt_internal_add_shaders(QuickVectorImageHelpers "vectorimage_shaders_comp_arithmetic"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    MULTIVIEW
    DEFINES
        "COMPOSITE_ARITHMETIC"
    PREFIX
        "/qt-project.org/quickvectorimage/"
    FILES
        "helpers/shaders_ng/fecomposite.frag"
    OUTPUTS
        "helpers/shaders_ng/fecompositearithmetic.frag.qsb"
)

qt_internal_add_shaders(QuickVectorImageHelpers "vectorimage_shaders_blend_normal"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    MULTIVIEW
    DEFINES
        "BLEND_NORMAL"
    PREFIX
        "/qt-project.org/quickvectorimage/"
    FILES
        "helpers/shaders_ng/feblend.frag"
    OUTPUTS
        "helpers/shaders_ng/feblendnormal.frag.qsb"
)

qt_internal_add_shaders(QuickVectorImageHelpers "vectorimage_shaders_blend_multiply"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    MULTIVIEW
    DEFINES
        "BLEND_MULTIPLY"
    PREFIX
        "/qt-project.org/quickvectorimage/"
    FILES
        "helpers/shaders_ng/feblend.frag"
    OUTPUTS
        "helpers/shaders_ng/feblendmultiply.frag.qsb"
)

qt_internal_add_shaders(QuickVectorImageHelpers "vectorimage_shaders_blend_screen"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    MULTIVIEW
    DEFINES
        "BLEND_SCREEN"
    PREFIX
        "/qt-project.org/quickvectorimage/"
    FILES
        "helpers/shaders_ng/feblend.frag"
    OUTPUTS
        "helpers/shaders_ng/feblendscreen.frag.qsb"
)

qt_internal_add_shaders(QuickVectorImageHelpers "vectorimage_shaders_blend_lighten"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    MULTIVIEW
    DEFINES
        "BLEND_LIGHTEN"
    PREFIX
        "/qt-project.org/quickvectorimage/"
    FILES
        "helpers/shaders_ng/feblend.frag"
    OUTPUTS
        "helpers/shaders_ng/feblendlighten.frag.qsb"
)

qt_internal_add_shaders(QuickVectorImageHelpers "vectorimage_shaders_blend_darken"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    MULTIVIEW
    DEFINES
        "BLEND_DARKEN"
    PREFIX
        "/qt-project.org/quickvectorimage/"
    FILES
        "helpers/shaders_ng/feblend.frag"
    OUTPUTS
        "helpers/shaders_ng/feblenddarken.frag.qsb"
)
