// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Effects
import "CustomMultiEffect"

Rectangle {
    id: mainWindow

    // Multiplier for resolution independency
    readonly property real dp: 0.2 + Math.min(width, height) / 1200

    width: 1280
    height: 720
    color: "#404040"

    Settings {
        id: settings
        onReseted: {
            settingsView.resetSettings();
        }
    }

    Settings {
        id: defaultSettings
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (resetSettingsOverlay.showAnimationProgress == 0)
                settings.animateMovement = !settings.animateMovement;
        }
        onPressed: {
            resetSettingsOverlay.startShow();
        }
        onReleased: {
            resetSettingsOverlay.stopShow();
        }
    }


    Item {
        id: mainArea
        anchors.left: settingsView.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        TestSourceItem {
            id: testSourceItem
            anchors.centerIn: parent
            width: parent.width / 2
            height: parent.height / 2

            layer.enabled: true
            visible: false
        }
        TestMaskItem {
            id: testMaskItem
            anchors.fill: testSourceItem
        }

        Rectangle {
            readonly property int margin: 2
            x: quickMultiEffect.x + quickMultiEffect.itemRect.x - margin
            y: quickMultiEffect.y + quickMultiEffect.itemRect.y - margin
            width: quickMultiEffect.itemRect.width + margin * 2
            height: quickMultiEffect.itemRect.height + margin * 2
            visible: settings.showItemSize
            color: "transparent"
            border.color: "#ffffff"
            border.width: 2
        }

        MultiEffect {
            id: quickMultiEffect
            anchors.fill: testSourceItem
            visible: !settings.showCustomMultiEffect
            source: testSourceItem
            maskSource: testMaskItem
            autoPaddingEnabled: settings.autoPaddingEnabled
            paddingRect: settings.paddingRect
            brightness: settings.brightnessEnabled ? settings.brightness : 0
            contrast:  settings.contrastEnabled ? settings.contrast : 0
            saturation: settings.saturationEnabled ? settings.saturation : 0
            colorizationColor: settings.colorizationColor
            colorization: settings.colorizationEnabled ? settings.colorization : 0
            blurEnabled: settings.blurEnabled
            blur: settings.blur
            blurMax: settings.blurMax
            blurMultiplier: settings.blurMultiplier
            shadowEnabled: settings.shadowEnabled
            shadowOpacity: settings.shadowOpacity
            shadowBlur: settings.shadowBlur
            shadowHorizontalOffset: settings.shadowHorizontalOffset
            shadowVerticalOffset: settings.shadowVerticalOffset
            shadowColor: settings.shadowColor
            shadowScale: settings.shadowScale
            maskEnabled: settings.maskEnabled
            maskInverted: settings.maskInverted
            maskThresholdMin: settings.maskThresholdMin
            maskSpreadAtMin: settings.maskSpreadAtMin
            maskThresholdMax: settings.maskThresholdMax
            maskSpreadAtMax: settings.maskSpreadAtMax

            onItemSizeChanged: {
                if (visible)
                    warningsView.showSizeWarning();
            }
            onShaderChanged: {
                if (visible)
                    warningsView.showShaderWarning();
            }
        }

        // For comparison, custom effect created with QQEM and the MultiEffect node.
        CustomMultiEffect {
            id: customMultiEffect
            anchors.fill: testSourceItem
            visible: settings.showCustomMultiEffect
            source: testSourceItem
            maskSource: testMaskItem
            brightness: settings.brightnessEnabled ? settings.brightness : 0
            contrast:  settings.contrastEnabled ? settings.contrast : 0
            saturation: settings.saturationEnabled ? settings.saturation : 0
            colorizationColor: settings.colorizationColor
            colorization: settings.colorizationEnabled ? settings.colorization : 0
            blur: settings.blur
            blurMax: settings.blurMax
            blurMultiplier: settings.blurMultiplier
            shadowOpacity: settings.shadowOpacity
            shadowBlur: settings.shadowBlur
            shadowHorizontalOffset: settings.shadowHorizontalOffset
            shadowVerticalOffset: settings.shadowVerticalOffset
            shadowColor: settings.shadowColor
            shadowScale: settings.shadowScale
            maskInverted: settings.maskInverted
            maskThresholdMin: settings.maskThresholdMin
            maskSpreadAtMin: settings.maskSpreadAtMin
            maskThresholdMax: settings.maskThresholdMax
            maskSpreadAtMax: settings.maskSpreadAtMax
        }
    }

    SettingsView {
        id: settingsView
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 20
        visible: settings.showSettingsView
        Component.onCompleted: {
            settings.reset();
        }
    }

    FpsItem {
        anchors.right: parent.right
    }

    ShaderView {
        id: shaderView
        visible: settings.showShader
        anchors.horizontalCenter: mainArea.horizontalCenter
        anchors.top: mainArea.top
        anchors.topMargin: 20
        text: "Fragment shader: " + quickMultiEffect.fragmentShader + "\nVertex shader: " + quickMultiEffect.vertexShader
    }

    WarningsView {
        id: warningsView
        anchors.bottom: parent.bottom
        anchors.left: settingsView.right
        anchors.leftMargin: 40
        anchors.right: parent.right
    }

    ResetSettingsOverlay {
        id: resetSettingsOverlay
        onAnimationFinished: {
            settings.reset();
        }
    }
}
