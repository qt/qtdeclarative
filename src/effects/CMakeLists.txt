# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QuickEffects Module:
#####################################################################

qt_internal_add_qml_module(QuickEffectsPrivate
    URI "QtQuick.Effects"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_TARGET effectsplugin
    CLASS_NAME QtQuickEffectsPlugin
    DEPENDENCIES
        QtQuick/auto
    INTERNAL_MODULE
    SOURCES
        qquickmultieffect.cpp qquickmultieffect_p.h qquickmultieffect_p_p.h
        qgfxsourceproxy.cpp qgfxsourceproxy_p.h
        qtquickeffectsglobal_p.h
    DEFINES
        QT_NO_INTEGER_EVENT_COORDINATES
        QT_NO_URL_CAST_FROM_STRING
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)

qt_internal_add_resource(QuickEffectsPrivate "effects"
    PREFIX
        "/"
    FILES
        "data/BlurItem.qml"
)

qt_internal_extend_target(QuickEffectsPrivate CONDITION MSVC
    DEFINES
        _CRT_SECURE_NO_WARNINGS
)

qt_internal_extend_target(QuickEffectsPrivate CONDITION EXISTS "qqml_enable_gcov"
    LIBRARIES
        gcov
    COMPILE_OPTIONS
        -fno-elide-constructors
        -fprofile-arcs
        -ftest-coverage
)

set(EFFECT_SHADER_ARGS
        SILENT
        PRECOMPILE
        OPTIMIZED
        PREFIX
            "/"
)

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders1"
    ${EFFECT_SHADER_ARGS}
    BATCHABLE
    FILES
        "data/shaders/bluritems.vert"
        "data/shaders/bluritems.frag"
)

# Multieffect fragment shaders
# c = Common color effects
# m = Mask
# b = Blur
# s = Shadow
# [n] = Amount of blur items used

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders2"
    ${EFFECT_SHADER_ARGS}
    BATCHABLE
    FILES
        "data/shaders/multieffect.vert"
    OUTPUTS
        "data/shaders/multieffect_c.vert.qsb"
)

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders3"
    ${EFFECT_SHADER_ARGS}
    BATCHABLE
    FILES
        "data/shaders/multieffect.vert"
    OUTPUTS
        "data/shaders/multieffect_cs.vert.qsb"
    DEFINES
        SHADOW
)

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders4"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_c0.frag.qsb"
)

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders5"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cm0.frag.qsb"
    DEFINES
        MASK
)

# Special shaders for non-blurred shadows (cs0 and cms0)
qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders6"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cs0.frag.qsb"
    DEFINES
        SHADOW
        BL0
)

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders7"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cms0.frag.qsb"
    DEFINES
        MASK
        SHADOW
        BL0
)

# Shaders for different blur levels
qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders8"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cb1.frag.qsb"
    DEFINES
        BLUR
        BL1
)

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders9"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cs1.frag.qsb"
    DEFINES
        SHADOW
        BL1
)

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders10"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cmb1.frag.qsb"
    DEFINES
        MASK
        BLUR
        BL1
)

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders11"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cms1.frag.qsb"
    DEFINES
        MASK
        SHADOW
        BL1
)

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders12"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cbs1.frag.qsb"
    DEFINES
        BLUR
        SHADOW
        BL1
)

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders13"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cmbs1.frag.qsb"
    DEFINES
        MASK
        BLUR
        SHADOW
        BL1
)

# level 2
qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders14"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cb2.frag.qsb"
    DEFINES
        BLUR
        BL1
        BL2
)

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders15"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cs2.frag.qsb"
    DEFINES
        SHADOW
        BL1
        BL2
)

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders16"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cmb2.frag.qsb"
    DEFINES
        MASK
        BLUR
        BL1
        BL2
)

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders17"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cms2.frag.qsb"
    DEFINES
        MASK
        SHADOW
        BL1
        BL2
)

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders18"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cbs2.frag.qsb"
    DEFINES
        BLUR
        SHADOW
        BL1
        BL2
)

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders19"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cmbs2.frag.qsb"
    DEFINES
        MASK
        BLUR
        SHADOW
        BL1
        BL2
)

# level 3
qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders20"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cb3.frag.qsb"
    DEFINES
        BLUR
        BL1
        BL2
        BL3
)

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders21"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cs3.frag.qsb"
    DEFINES
        SHADOW
        BL1
        BL2
        BL3
)

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders22"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cmb3.frag.qsb"
    DEFINES
        MASK
        BLUR
        BL1
        BL2
        BL3
)

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders23"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cms3.frag.qsb"
    DEFINES
        MASK
        SHADOW
        BL1
        BL2
        BL3
)

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders24"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cbs3.frag.qsb"
    DEFINES
        BLUR
        SHADOW
        BL1
        BL2
        BL3
)

qt_internal_add_shaders(QuickEffectsPrivate "multieffect_shaders25"
    ${EFFECT_SHADER_ARGS}
    FILES
        "data/shaders/multieffect.frag"
    OUTPUTS
        "data/shaders/multieffect_cmbs3.frag.qsb"
    DEFINES
        MASK
        BLUR
        SHADOW
        BL1
        BL2
        BL3
)
