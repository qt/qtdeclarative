# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from nodetypes_ng.pro.

#####################################################################
## nodetypes_ng Binary:
#####################################################################

qt_internal_add_manual_test(nodetypes_ng
    GUI
    SOURCES
        nodetypes_ng.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
set(nodetypes_ng_resource_files
    "AtlasedImages.qml"
    "CompressedImages.qml"
    "DistanceFieldText.qml"
    "Images.qml"
    "Layers.qml"
    "LotsOfNodes.qml"
    "LotsOfRects.qml"
    "MoreWindows.qml"
    "MultiClipRects.qml"
    "Painter.qml"
    "Rects.qml"
    "ShaderEffect.qml"
    "ShaderEffectNoAnim.qml"
    "ShaderEffectSource.qml"
    "SimpleRect.qml"
    "Text.qml"
    "arrow-down.png"
    "arrow-up.png"
    "blacknwhite.png"
    "car_etc2_nomips.ktx"
    "face-smile.png"
    "main.qml"
    "minus-sign.png"
    "plus-sign.png"
    "qt.png"
    "qt_bc1_10mips.ktx"
    "shadow_pass1.frag.qsb"
    "shadow_pass1_legacy_gl.frag"
    "shadow_pass2.frag.qsb"
    "shadow_pass2_legacy_gl.frag"
    "wobble.frag.qsb"
    "wobble.vert.qsb"
    "wobble_legacy_gl.frag"
    "wobble_legacy_gl.vert"
)

qt_internal_add_resource(nodetypes_ng "nodetypes_ng"
    PREFIX
        "/"
    FILES
        ${nodetypes_ng_resource_files}
)


#### Keys ignored in scope 1:.:.:nodetypes_ng.pro:<TRUE>:
# OTHER_FILES = "main.qml" "SimpleRect.qml" "Rects.qml" "LotsOfRects.qml"
