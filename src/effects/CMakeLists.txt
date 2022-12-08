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

# Resources:
set(effects_resource_files
    "data/BlurItem.qml"
    "data/shaders/bluritems.vert.qsb"
    "data/shaders/bluritems.frag.qsb"
    "data/shaders/multieffect_c.vert.qsb"
    "data/shaders/multieffect_cs.vert.qsb"
    "data/shaders/multieffect_c0.frag.qsb"
    "data/shaders/multieffect_cb1.frag.qsb"
    "data/shaders/multieffect_cb2.frag.qsb"
    "data/shaders/multieffect_cb3.frag.qsb"
    "data/shaders/multieffect_cbs1.frag.qsb"
    "data/shaders/multieffect_cbs2.frag.qsb"
    "data/shaders/multieffect_cbs3.frag.qsb"
    "data/shaders/multieffect_cm0.frag.qsb"
    "data/shaders/multieffect_cmb1.frag.qsb"
    "data/shaders/multieffect_cmb2.frag.qsb"
    "data/shaders/multieffect_cmb3.frag.qsb"
    "data/shaders/multieffect_cmbs1.frag.qsb"
    "data/shaders/multieffect_cmbs2.frag.qsb"
    "data/shaders/multieffect_cmbs3.frag.qsb"
    "data/shaders/multieffect_cms0.frag.qsb"
    "data/shaders/multieffect_cms1.frag.qsb"
    "data/shaders/multieffect_cms2.frag.qsb"
    "data/shaders/multieffect_cms3.frag.qsb"
    "data/shaders/multieffect_cs0.frag.qsb"
    "data/shaders/multieffect_cs1.frag.qsb"
    "data/shaders/multieffect_cs2.frag.qsb"
    "data/shaders/multieffect_cs3.frag.qsb"
)

qt_internal_add_resource(QuickEffectsPrivate "effects"
    PREFIX
        "/"
    FILES
        ${effects_resource_files}
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
