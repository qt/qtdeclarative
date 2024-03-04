// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SceneGraphRendering

Item {
    width: 800
    height: 600

    // The checkers background
    ShaderEffect {
        id: tileBackground
        anchors.fill: parent

        property real tileSize: 16
        property color color1: Qt.rgba(0.9, 0.9, 0.9, 1);
        property color color2: Qt.rgba(0.85, 0.85, 0.85, 1);

        property size pixelSize: Qt.size(width / tileSize, height / tileSize);

        fragmentShader: "qrc:/scenegraph/rhitextureitem/shaders/checker.frag.qsb"
    }

    //! [0]
    ExampleRhiItem {
        id: renderer
        anchors.fill: parent
        anchors.margins: 10

        mirrorVertically: cbMirror.checked
        sampleCount: cbMSAA.checked ? 4 : 1
        fixedColorBufferWidth: cbSize.checked ? slSize.value.toFixed(0) : 0
        fixedColorBufferHeight: cbSize.checked ? slSize.value.toFixed(0) : 0
        alphaBlending: cbBlend.checked
        colorBufferFormat: rdFormatRGBA8.checked ? ExampleRhiItem.TextureFormat.RGBA8
                                             : rdFormatFP16.checked ? ExampleRhiItem.TextureFormat.RGBA16F
                                                                    : rdFormatFP32.checked ? ExampleRhiItem.TextureFormat.RGBA32F
                                                                                           : ExampleRhiItem.TextureFormat.RGB10A2

        // custom properties provided by the QQuickRhiItem subclass: angle, backgroundAlpha
        NumberAnimation on angle {
            from: 0
            to: 360
            duration: 5000
            loops: Animation.Infinite
            running: cbAnimate.checked
        }

        backgroundAlpha: slAlpha.value
    //! [0]

        // The transform is just to show something interesting..
        transform: [
            Rotation { id: rotation; axis.x: 0; axis.z: 0; axis.y: 1; angle: 0; origin.x: renderer.width / 2; origin.y: renderer.height / 2; },
            Translate { id: txOut; x: -renderer.width / 2; y: -renderer.height / 2 },
            Scale { id: scale; },
            Translate { id: txIn; x: renderer.width / 2; y: renderer.height / 2 }
        ]
    }

    // Just to show something interesting
    SequentialAnimation {
        PauseAnimation { duration: 5000 }
        ParallelAnimation {
            NumberAnimation { target: scale; property: "xScale"; to: 0.6; duration: 1000; easing.type: Easing.InOutBack }
            NumberAnimation { target: scale; property: "yScale"; to: 0.6; duration: 1000; easing.type: Easing.InOutBack }
        }
        NumberAnimation { target: rotation; property: "angle"; to: 80; duration: 1000; easing.type: Easing.InOutCubic }
        NumberAnimation { target: rotation; property: "angle"; to: -80; duration: 1000; easing.type: Easing.InOutCubic }
        NumberAnimation { target: rotation; property: "angle"; to: 0; duration: 1000; easing.type: Easing.InOutCubic }
        NumberAnimation { target: renderer; property: "opacity"; to: 0.5; duration: 1000; easing.type: Easing.InOutCubic }
        PauseAnimation { duration: 1000 }
        NumberAnimation { target: renderer; property: "opacity"; to: 1.0; duration: 1000; easing.type: Easing.InOutCubic }
        ParallelAnimation {
            NumberAnimation { target: scale; property: "xScale"; to: 1; duration: 1000; easing.type: Easing.InOutBack }
            NumberAnimation { target: scale; property: "yScale"; to: 1; duration: 1000; easing.type: Easing.InOutBack }
        }
        running: cbAnimate.checked
        loops: Animation.Infinite
    }

    Rectangle {
        id: labelFrame
        anchors.margins: -10
        radius: 5
        color: "white"
        border.color: "black"
        anchors.fill: label
    }

    Text {
        id: label
        anchors.bottom: renderer.bottom
        anchors.left: renderer.left
        anchors.right: renderer.right
        anchors.margins: 20
        wrapMode: Text.WordWrap
        text: qsTr("The triangle on the blue background is rendered by working directly the QRhi APIs on the scene graph render thread. The custom QQuickItem subclasses QQuickRhiItem, and it in effect draws a quad textured with the QRhiTexture containing the custom 3D rendering. Thanks to being a true QQuickItem, 2D/2.5D transforms, blending/opacity, stacking, clipping, etc. all work as usual.")
    }

    RowLayout {
        anchors.top: renderer.top
        anchors.right: renderer.right
        anchors.margins: 10
        Item {
            id: settingsButton
            implicitWidth: 64
            implicitHeight: 64
            Image {
                anchors.centerIn: parent
                source: "icon_settings.png"
            }
            HoverHandler {
                id: hoverHandler
            }
        }
        Text {
            Layout.alignment: Qt.AlignVCenter
            text: settingsDrawer.title
            color: "white"
            font.pointSize: 16
        }
        TapHandler {
            // qmllint disable signal-handler-parameters
            onTapped: settingsDrawer.isOpen = !settingsDrawer.isOpen;
            // qmllint enable signal-handler-parameters
        }
    }

    SettingsDrawer {
        id: settingsDrawer
        title: qsTr("Settings")
        isOpen: false
        isLandscape: true
        width: isLandscape ? implicitWidth * 1.2 : parent.width
        height: isLandscape ? parent.height * 0.8 : parent.height * 0.33

        CheckBox {
            id: cbAnimate
            text: qsTr("Animate")
            checked: true
        }

        CheckBox {
            id: cbMirror
            text: qsTr("Mirror vertically")
            checked: false
        }

        CheckBox {
            id: cbMSAA
            text: qsTr("Enable 4x MSAA")
            checked: false
        }

        CheckBox {
            id: cbSize
            text: qsTr("Use fixed size")
            checked: false
        }
        RowLayout {
            Label {
                text: qsTr("Texture size")
            }
            Slider {
                id: slSize
                enabled: cbSize.checked
                from: 8
                to: 2048
                value: 128
                Layout.fillWidth: false
            }
        }

        Label {
            text: qsTr("Backing texture size: %1x%2 pixels").arg(renderer.effectiveColorBufferSize.width).arg(renderer.effectiveColorBufferSize.height)
        }

        Label {
            text: qsTr("Item logical size: %1x%2").arg(renderer.width).arg(renderer.height)
        }

        CheckBox {
            id: cbBlend
            text: qsTr("Force alpha blending\nregardless of item opacity")
            checked: false
        }
        RowLayout {
            Label {
                text: qsTr("Background alpha")
            }
            Slider {
                id: slAlpha
                from: 0
                to: 1.0
                value: 1.0
                Layout.fillWidth: false
            }
        }

        Label {
            text: qsTr("Background alpha: %1").arg(renderer.backgroundAlpha.toFixed(2))
        }

        Label {
            text: qsTr("Item opacity: %1").arg(renderer.opacity.toFixed(2))
        }

        Label {
            property string apiName:
                if (GraphicsInfo.api === GraphicsInfo.OpenGL)
                    "OpenGL";
                else if (GraphicsInfo.api === GraphicsInfo.Direct3D11)
                    "D3D11";
                else if (GraphicsInfo.api === GraphicsInfo.Direct3D12)
                    "D3D12";
                else if (GraphicsInfo.api === GraphicsInfo.Vulkan)
                    "Vulkan";
                else if (GraphicsInfo.api === GraphicsInfo.Metal)
                    "Metal";
                else if (GraphicsInfo.api === GraphicsInfo.Null)
                    "Null";
                else
                    "Unknown API";
            text: "The underlying graphics API is " + apiName
        }

        GroupBox {
            title: qsTr("Texture format")
            ColumnLayout {
                RadioButton {
                    id: rdFormatRGBA8
                    text: qsTr("8-bit RGBA")
                    checked: true
                    Layout.fillWidth: false
                }
                RadioButton {
                    id: rdFormatFP16
                    text: qsTr("Half-float RGBA")
                    Layout.fillWidth: false
                }
                RadioButton {
                    id: rdFormatFP32
                    text: qsTr("Float RGBA")
                    Layout.fillWidth: false
                }
                RadioButton {
                    id: rdFormatRGB10A2
                    text: qsTr("10-bit RGB, 2-bit A")
                    Layout.fillWidth: false
                }
            }
        }
    }
}
