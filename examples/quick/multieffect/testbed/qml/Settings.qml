// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

QtObject {
    id: rootItem

    // Emitted when settings are reseted to default
    signal reseted

    // When adding settings here remember to add them also into reset()

    // *** General settings - No UI for these ***
    // Change to false to not show settings view at all
    property bool showSettingsView: true
    property real settingsViewWidth: 100 + 150 * dp
    property bool animateMovement: true
    property bool showShader: false
    property bool showItemSize: false
    property bool showCustomMultiEffect: false

    property bool autoPaddingEnabled: true
    property rect paddingRect: Qt.rect(0, 0, 0, 0)

    property bool brightnessEnabled: true
    property real brightness: 0.0
    property bool contrastEnabled: true
    property real contrast: 0.0
    property bool saturationEnabled: true
    property real saturation: 0.0
    property bool colorizationEnabled: true
    property color colorizationColor: Qt.rgba(1.0, 0.0, 0.0, 1.0)
    property real colorization: 0.0

    property bool blurEnabled: true
    property real blur: 0.0
    property int blurMax: 32
    property real blurMultiplier: 0.0

    property bool shadowEnabled: true
    property real shadowOpacity: 1.0
    property real shadowBlur: 1.0
    property real shadowHorizontalOffset: 10
    property real shadowVerticalOffset: 5
    property color shadowColor: Qt.rgba(0.0, 0.0, 0.0, 1.0)
    property real shadowScale: 1.0

    property bool maskEnabled: true
    property bool maskInverted: false
    property real maskThresholdMin: 0.0
    property real maskSpreadAtMin: 0.0
    property real maskThresholdMax: 1.0
    property real maskSpreadAtMax: 0.0

    function reset() {
        autoPaddingEnabled = defaultSettings.autoPaddingEnabled;
        paddingRect = defaultSettings.paddingRect;

        brightnessEnabled = defaultSettings.brightnessEnabled;
        brightness = defaultSettings.brightness;
        contrastEnabled = defaultSettings.contrastEnabled;
        contrast = defaultSettings.contrast;
        saturationEnabled = defaultSettings.saturationEnabled;
        saturation = defaultSettings.saturation;
        colorizationEnabled = defaultSettings.colorizationEnabled;
        colorizationColor = defaultSettings.colorizationColor;
        colorization = defaultSettings.colorization;

        blurEnabled = defaultSettings.blurEnabled;
        blur = defaultSettings.blur;
        blurMax = defaultSettings.blurMax;
        blurMultiplier = defaultSettings.blurMultiplier;

        shadowEnabled = defaultSettings.shadowEnabled;
        shadowOpacity = defaultSettings.shadowOpacity;
        shadowBlur = defaultSettings.shadowBlur;
        shadowHorizontalOffset = defaultSettings.shadowHorizontalOffset;
        shadowVerticalOffset = defaultSettings.shadowVerticalOffset;
        shadowColor = defaultSettings.shadowColor;
        shadowScale = defaultSettings.shadowScale;

        maskEnabled = defaultSettings.maskEnabled;
        maskInverted = defaultSettings.maskInverted;
        maskThresholdMin = defaultSettings.maskThresholdMin;
        maskSpreadAtMin = defaultSettings.maskSpreadAtMin;
        maskThresholdMax = defaultSettings.maskThresholdMax;
        maskSpreadAtMax = defaultSettings.maskSpreadAtMax;

        rootItem.reseted();
    }
}
