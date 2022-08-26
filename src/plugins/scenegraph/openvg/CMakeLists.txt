# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from openvg.pro.

#####################################################################
## QSGOpenVGAdaptation Plugin:
#####################################################################

qt_internal_add_plugin(QSGOpenVGAdaptationPlugin
    OUTPUT_NAME qsgopenvgbackend
    CLASS_NAME QSGOpenVGAdaptation
    PLUGIN_TYPE scenegraph
    TARGET_PRODUCT "Qt Quick OpenVG Renderer (Qt $$QT_VERSION)"
    TARGET_DESCRIPTION "Quick OpenVG Renderer for Qt."
    SOURCES
        qopenvgcontext.cpp qopenvgcontext_p.h
        qopenvgmatrix.cpp qopenvgmatrix.h
        qopenvgoffscreensurface.cpp qopenvgoffscreensurface.h
        qsgopenvgadaptation.cpp qsgopenvgadaptation_p.h
        qsgopenvgcontext.cpp qsgopenvgcontext_p.h
        qsgopenvgfontglyphcache.cpp qsgopenvgfontglyphcache.h
        qsgopenvgglyphnode.cpp qsgopenvgglyphnode_p.h
        qsgopenvghelpers.cpp qsgopenvghelpers.h
        qsgopenvginternalimagenode.cpp qsgopenvginternalimagenode.h
        qsgopenvginternalrectanglenode.cpp qsgopenvginternalrectanglenode.h
        qsgopenvglayer.cpp qsgopenvglayer.h
        qsgopenvgnodevisitor.cpp qsgopenvgnodevisitor.h
        qsgopenvgpainternode.cpp qsgopenvgpainternode.h
        qsgopenvgpublicnodes.cpp qsgopenvgpublicnodes.h
        qsgopenvgrenderable.cpp qsgopenvgrenderable.h
        qsgopenvgrenderer.cpp qsgopenvgrenderer_p.h
        qsgopenvgrenderloop.cpp qsgopenvgrenderloop_p.h
        qsgopenvgtexture.cpp qsgopenvgtexture.h
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        openvg
)

#### Keys ignored in scope 1:.:.:openvg.pro:<TRUE>:
# OTHER_FILES = "$$PWD/openvg.json"
# QMAKE_TARGET_DESCRIPTION = "Quick OpenVG Renderer for Qt."
# QMAKE_TARGET_PRODUCT = "Qt Quick OpenVG Renderer (Qt $$QT_VERSION)"

## Scopes:
#####################################################################

qt_internal_extend_target(QSGOpenVGAdaptationPlugin CONDITION CMAKE_BUILD_TYPE STREQUAL Debug AND debug_and_release
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_BINARY_DIR}/../../../quick/.tracegen/debug
)

qt_internal_extend_target(QSGOpenVGAdaptationPlugin CONDITION debug_and_release AND NOT CMAKE_BUILD_TYPE STREQUAL Debug
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_BINARY_DIR}/../../../quick/.tracegen/release
)

qt_internal_extend_target(QSGOpenVGAdaptationPlugin CONDITION NOT debug_and_release
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_BINARY_DIR}/../../../quick/.tracegen
)

qt_internal_extend_target(QSGOpenVGAdaptationPlugin CONDITION QT_FEATURE_quick_sprite
    SOURCES
        qsgopenvgspritenode.cpp qsgopenvgspritenode.h
)
