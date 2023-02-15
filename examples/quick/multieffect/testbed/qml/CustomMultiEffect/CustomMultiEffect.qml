// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
// Created with Qt Quick Effect Maker (version 0.43), Thu Feb 16 13:48:55 2023

import QtQuick

Item {
    id: rootItem

    // This is the main source for the effect
    property Item source: null

    property real contrast: 0
    property real brightness: 0
    property real saturation: 0
    property real colorization: 0
    property color colorizationColor: Qt.rgba(1, 0, 0, 1)
    property int blurMax: 32
    property real blur: 0
    // This property defines how much blur (radius) is applied to the shadow.
    //
    // The value ranges from 0.0 (no blur) to 1.0 (full blur). By default, the property is set to \c 0.0 (no change). The amount of full blur is affected by blurHelperBlurMultiplier.
    property real shadowBlur: 1
    property real shadowOpacity: 1
    property color shadowColor: Qt.rgba(0, 0, 0, 1)
    property real shadowScale: 1
    property real shadowHorizontalOffset: 0
    property real shadowVerticalOffset: 0
    // Source item for the mask effect. By default the alpha channel of the source item is used for masking but this can be easily adjusted in the shader.
    property var maskSource: null
    // This property defines a lower threshold value for the mask pixels. The mask pixels that have an alpha value below this property are used to completely mask away the corresponding pixels from the source item. The mask pixels that have a higher alpha value are used to alphablend the source item to the display.
    //
    // The value ranges from 0.0 (alpha value 0) to 1.0 (alpha value 255). By default, the property is set to 0.0.
    property real maskThresholdMin: 0
    // This property defines the smoothness of the mask edges near the maskThresholdMin. Setting higher spread values softens the transition from the transparent mask pixels towards opaque mask pixels by adding interpolated values between them.
    //
    // The value ranges from 0.0 (sharp mask edge) to 1.0 (smooth mask edge). By default, the property is set to 0.0.
    property real maskSpreadAtMin: 0
    // This property defines an upper threshold value for the mask pixels. The mask pixels that have an alpha value below this property are used to completely mask away the corresponding pixels from the source item. The mask pixels that have a higher alpha value are used to alphablend the source item to the display.
    //
    // The value ranges from 0.0 (alpha value 0) to 1.0 (alpha value 255). By default, the property is set to 1.0.
    property real maskThresholdMax: 1
    // This property defines the smoothness of the mask edges near the maskThresholdMax. Using higher spread values softens the transition from the transparent mask pixels towards opaque mask pixels by adding interpolated values between them.
    //
    // The value ranges from 0.0 (sharp mask edge) to 1.0 (smooth mask edge). By default, the property is set to 0.0.
    property real maskSpreadAtMax: 0
    // This property switches the mask to the opposite side; instead of masking away the content outside maskThresholdMin and maskThresholdMax, content between them will get masked away.
    //
    // By default, the property is set to false.
    property bool maskInverted: false
    // This property defines a multiplier for extending the blur radius.
    //
    // The value ranges from 0.0 (not multiplied) to inf. By default, the property is set to 0.0. Incresing the multiplier extends the blur radius, but decreases the blur quality. This is more performant option for a bigger blur radius than BLUR_HELPER_MAX_LEVEL as it doesn't increase the amount of texture lookups.
    //
    // Note: This affects to both blur and shadow effects.
    property real blurMultiplier: 0

    BlurHelper {
        id: blurHelper
        anchors.fill: parent
        property int blurMax: 64
        property real blurMultiplier: rootItem.blurMultiplier
    }
    ShaderEffect {
        readonly property alias iSource: rootItem.source
        readonly property vector3d iResolution: Qt.vector3d(width, height, 1.0)
        readonly property alias iSourceBlur1: blurHelper.blurSrc1
        readonly property alias iSourceBlur2: blurHelper.blurSrc2
        readonly property alias iSourceBlur3: blurHelper.blurSrc3
        readonly property alias iSourceBlur4: blurHelper.blurSrc4
        readonly property alias iSourceBlur5: blurHelper.blurSrc5
        readonly property alias contrast: rootItem.contrast
        readonly property alias brightness: rootItem.brightness
        readonly property alias saturation: rootItem.saturation
        readonly property alias colorization: rootItem.colorization
        readonly property alias colorizationColor: rootItem.colorizationColor
        readonly property alias blurMax: rootItem.blurMax
        readonly property alias blur: rootItem.blur
        readonly property alias shadowBlur: rootItem.shadowBlur
        readonly property alias shadowOpacity: rootItem.shadowOpacity
        readonly property alias shadowColor: rootItem.shadowColor
        readonly property alias shadowScale: rootItem.shadowScale
        readonly property alias shadowHorizontalOffset: rootItem.shadowHorizontalOffset
        readonly property alias shadowVerticalOffset: rootItem.shadowVerticalOffset
        readonly property alias maskSource: rootItem.maskSource
        readonly property alias maskThresholdMin: rootItem.maskThresholdMin
        readonly property alias maskSpreadAtMin: rootItem.maskSpreadAtMin
        readonly property alias maskThresholdMax: rootItem.maskThresholdMax
        readonly property alias maskSpreadAtMax: rootItem.maskSpreadAtMax
        readonly property alias maskInverted: rootItem.maskInverted
        readonly property alias blurMultiplier: rootItem.blurMultiplier

        vertexShader: 'custommultieffect.vert.qsb'
        fragmentShader: 'custommultieffect.frag.qsb'
        anchors.fill: parent
    }
}
