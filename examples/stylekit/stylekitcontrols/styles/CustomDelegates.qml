// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes
import QtQuick.Templates as T
import Qt.labs.StyleKit

Style {

    /******************************************************************
     * Defining some helper types for the custom delegates further down
     ******************************************************************/

    component Star : Shape {
        id: star
        ShapePath {
            fillColor: star.palette.accent
            scale: Qt.size(star.width, star.height)
            PathMove { x: 0.50; y: 0.00 }
            PathLine { x: 0.59; y: 0.35 }
            PathLine { x: 0.97; y: 0.35 }
            PathLine { x: 0.66; y: 0.57 }
            PathLine { x: 0.78; y: 0.91 }
            PathLine { x: 0.50; y: 0.70 }
            PathLine { x: 0.22; y: 0.91 }
            PathLine { x: 0.34; y: 0.57 }
            PathLine { x: 0.03; y: 0.35 }
            PathLine { x: 0.41; y: 0.35 }
            PathLine { x: 0.50; y: 0.00 }
        }
        NumberAnimation on rotation {
            loops: Animation.Infinite
            from: 0
            to: 359
            duration: 20000
        }
    }

    component OverlayData: QtObject {
        property real overlayScale: 1
    }

    /******************************************************************
     * Define custom delegates. These replace the default StyledItem
     * for selected controls in the style definition below.
     ******************************************************************/

    component OverlayDelegate : StyledItem {
        /* Using StyledItem as the base type is the easiest approach when creating
         * a custom delegate. A StyledItem will draw the delegate as configured by
         * the style, and give you the opportunity to place your own items on top. */
        id: delegate
        width: parent.width
        height: parent.height

        Star {
            width: 40
            height: 40
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -10
            scale: delegate.delegateStyle.data.overlayScale
            Behavior on scale {
                NumberAnimation {
                    duration: 300
                    easing.type: Easing.OutBounce
                }
            }
        }
        Text {
            text: "overlay"
            font.pixelSize: 8
            y: -10
        }
    }

    component UnderlayDelegate : Item {
        /* Custom delegates that don't inherit from StyledItem needs to define
         * 'delegateStyle' and 'control' properties, which are assinged to by StyleKit.
         * Use 'delegateStyle' to bind to style attributes like color, radius, and opacity.
         * Use 'control' to access the Quick Control the owns the delegate. */
        id: delegate
        required property DelegateStyle delegateStyle
        required property QtObject control

        implicitWidth: delegateStyle.implicitWidth
        implicitHeight: delegateStyle.implicitHeight
        width: parent.width
        height: parent.height
        scale: delegateStyle.scale
        rotation: delegateStyle.rotation
        visible: delegateStyle.visible

        Star {
            visible: delegate.control.checked
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -1
            width: 60
            height: 60
        }

        Text {
            text: "underlay"
            font.pixelSize: 8
            y: -10
        }

        StyledItem {
            // Embed a StyledItem to render the standard delegate on top
            delegateStyle: delegate.delegateStyle
        }
    }

    component SliderHandle : StyledItem {
        /* Unlike the 'data' property, which varies per state, you can use regular
         * QML properties to pass static information to a delegate. Here, 'isFirstHandle'
         * distinguishes the first from the second handle in a RangeSlider, and
         * 'control' gives access to the slider's current value(s). */
        id: sliderHandle
        property bool isFirstHandle: false
        required property QtObject control

        Text {
            rotation: sliderHandle.control.vertical ? -90 : 0
            color: "ghostwhite"
            anchors.centerIn: parent
            font.pixelSize: 9
            text: {
                if (sliderHandle.control instanceof T.RangeSlider) {
                    if (sliderHandle.isFirstHandle)
                        return sliderHandle.control.first.value.toFixed(0)
                    else
                        return sliderHandle.control.second.value.toFixed(0)
                }

                return sliderHandle.control.value.toFixed(0)
            }
        }
    }

    component NoiseDelegate : ShaderEffect {
        /* Use graphical effects in combination with StyledItem to create more
         * complex delegate appearances. In this delegate, we create a noise overlay. */
        implicitWidth: unifiedSourceItem.implicitWidth
        implicitHeight: unifiedSourceItem.implicitHeight
        width: parent.width
        height: parent.height

        required property DelegateStyle delegateStyle

        // The following properties are used by the shader (noise.frag)
        property size sourceItemSize: Qt.size(unifiedSourceItem.width, unifiedSourceItem.height)
        property color borderColor: delegateStyle.border.color
        property real borderMaskEnabled: 1
        property real borderMaskThreshold: 0.001
        property real particleDensity: 0.1
        property real particleSize: 1
        property color particleColor: "black"
        property Item source: ShaderEffectSource { live: true; sourceItem: unifiedSourceItem }
        property real particleOpacity: 0.4
        property real time

        // Note: noise.frag is compiled to noise.qsb from CMakeLists.txt
        fragmentShader: "qrc:/effects/noise.qsb"

        NumberAnimation on time {
            loops: Animation.Infinite
            from: 0
            to: Math.PI * 2
            duration: 1000
        }

        StyledItem {
            id: unifiedSourceItem
            delegateStyle: parent.delegateStyle
            width: parent.width
            height: parent.height
            visible: false
            rotation: 0.0
            scale: 1.0
        }

        Text {
            text: "noise"
            font.pixelSize: 8
            y: -10
        }
    }

    component WavingQt : ShaderEffect {
        implicitWidth: delegateStyle.implicitWidth
        implicitHeight: delegateStyle.implicitHeight
        visible: delegateStyle.visible

        required property DelegateStyle delegateStyle

        // The following properties are used by the shader (wave.frag)
        property real amplitude: 0.04 * 0.5
        property real frequency: 20
        property real time

        NumberAnimation on time {
            loops: Animation.Infinite
            from: 0
            to: Math.PI * 2
            duration: 600
        }

        // Note: wave.frag is compiled to wave.qsb from CMakeLists.txt
        fragmentShader: "qrc:/effects/wave.qsb"

        property Item sourceItem: ShaderEffectSource {
            sourceItem: Image {
                width: 40
                height: 40
                source: "qrc:/images/qt.png"
                visible: false
            }
        }
    }

    component CustomShadowDelegate : Item {
        required property DelegateStyle delegateStyle

        x: delegateStyle.shadow.verticalOffset
        y: delegateStyle.shadow.horizontalOffset
        width: parent.width
        height: parent.height

        Rectangle {
            width: parent.width
            height: parent.height
            radius: parent.delegateStyle.radius
            color: parent.delegateStyle.shadow.color
            opacity: parent.delegateStyle.shadow.opacity
        }

        Text {
            anchors.right: parent.right
            anchors.top: parent.bottom
            anchors.rightMargin: 10
            font.pixelSize: 8
            text: "Custom shadow"
        }
    }

    /******************************************************************
     * Define the style, assigning the custom delegates above to specific
     * controls in place of the default StyledItem:
     ******************************************************************/

    applicationWindow {
        background.color: "#544e52"
    }

    control {
        text.color: "ghostwhite"
        background {
            border.color: "#3d373b"
            shadow.color: "#555555"
            color: "#8e848a"
        }

        handle {
            color: "#8e848a"
            border.color: Qt.darker("#544e52", 1.5)
            shadow.color: "#808080"
        }

        indicator {
            color: Qt.darker("#8e848a", 1.6)
        }
        hovered.background.color: Qt.lighter("#8e848a", 1.2)
    }

    button {
        topPadding: 30
        background {
            delegate: OverlayDelegate{}
            // Use the 'data' property to pass custom, per-state information to a custom delegate
            data: OverlayData {
                overlayScale: 0.5
            }
        }
        hovered.background.data: OverlayData {
            overlayScale: 1.8
        }
        pressed.background.data: OverlayData {
            overlayScale: 1.6
        }
        checked.background.data: OverlayData {
            overlayScale: 1.4
        }
    }

    flatButton {
        background.shadow.visible: false
    }

    checkBox {
        indicator.foreground {
            implicitWidth: 30
            implicitHeight: 30
            margins: 4
            delegate: WavingQt {}
        }
    }

    radioButton {
        indicator.delegate: UnderlayDelegate {}
    }

    slider {
        background.visible: true
        // background.delegate: NoiseDelegate {}
        // indicator.delegate: NoiseDelegate {}
        handle.delegate: SliderHandle { isFirstHandle: true }
        handle.second.delegate: SliderHandle { isFirstHandle: false }
    }

    textField {
        background.shadow.verticalOffset: 4
        background.shadow.horizontalOffset: 4
        background.shadow.delegate: CustomShadowDelegate {}
    }

    switchControl {
        background.visible: true
        checked {
            background.delegate: NoiseDelegate {}
            indicator.foreground.delegate: NoiseDelegate {}
        }
    }

    comboBox {
        background.implicitWidth: 200
    }

    frame {
        padding: 20
        spacing: 50
        hovered.background.color: "#8e848a"
    }

}
