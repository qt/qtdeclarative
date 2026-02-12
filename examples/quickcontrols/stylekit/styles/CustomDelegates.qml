// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T
import Qt.labs.StyleKit

Style {

    // Define some custom delegates:

    component OverlayDelegate : StyledItem {
        // Using StyledItem as the base type is the easiest approach when creating
        // a custom delegate. A StyledItem will draw the delegate as configured by
        // the style, and give you the opportunity to place your own items on top.
        // Note that all delegates used in StyleKit, custom or not, are laid out by
        // StyleKit, so you only need to focus on the appearance.
        Rectangle {
            width: 20
            height: 15
            scale: 2.5
            radius: 255
            opacity: 0.5
            color: "transparent"
            border.width: 4
            border.color: palette.accent
        }
    }

    component UnderlayDelegate : Item {
        /* Custom delegates that don't inherit from StyledItem can optionally
         * declare 'delegateProperties' and 'control' properties. Use delegateProperties
         * to bind to style attributes like color, radius, and opacity. Use control
         * to access the Quick Control the owns the delegate. */
        required property StyleKitDelegateProperties delegateProperties
        required property QtObject control

        implicitWidth: delegateProperties.implicitWidth
        implicitHeight: delegateProperties.implicitHeight
        width: parent.width
        height: parent.height
        scale: delegateProperties.scale
        rotation: delegateProperties.rotation
        visible: delegateProperties.visible

        Rectangle {
            id: underlay
            anchors.centerIn: parent
            width: 10 + (parent.width / 2)
            height: 10 + (parent.height / 2)
            radius: delegateProperties.radius
            scale: delegateProperties.data.underlayScale
            opacity: 0.5
            border.width: 8
            border.color: palette.accent
            Behavior on scale {
                NumberAnimation {
                    duration: 100
                }
            }
            NumberAnimation on rotation {
                loops: Animation.Infinite
                from: 0
                to: 359
                duration: 5000
            }
        }

        StyledItem {
            // Embed a StyledItem to render the standard delegate on top of the custom one
            delegateProperties: parent.delegateProperties
        }
    }

    component SliderHandle : StyledItem {
        // You can pass your own properties from the style, like
        // here, where we use 'isFirst' to tell whether the delegate instance
        // represents the first or the second handle, in case of a RangeSlider.
        // By adding a 'control' property, we can access the slider's value(s).
        id: sliderHandle
        property bool isFirstHandle: false
        required property QtObject control

        Text {
            rotation: sliderHandle.control.vertical ? -90 : 0
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
        // Use graphical effects in combination with StyledItem to create more
        // complex delegate appearances. In this delegate, we create a noise overlay.
        implicitWidth: unifiedSourceItem.implicitWidth
        implicitHeight: unifiedSourceItem.implicitHeight
        width: parent.width
        height: parent.height

        required property StyleKitDelegateProperties delegateProperties

        // The following properties are used by the shader (noise.frag)
        property size sourceItemSize: Qt.size(unifiedSourceItem.width, unifiedSourceItem.height)
        property color borderColor: delegateProperties.border.color
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
            delegateProperties: parent.delegateProperties
            width: parent.width
            height: parent.height
            visible: false
            rotation: 0.0
            scale: 1.0
        }
    }

    component WavingQt : ShaderEffect {
        implicitWidth: delegateProperties.implicitWidth
        implicitHeight: delegateProperties.implicitHeight

        required property StyleKitDelegateProperties delegateProperties

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
        required property StyleKitDelegateProperties delegateProperties

        x: delegateProperties.shadow.verticalOffset
        y: delegateProperties.shadow.horizontalOffset
        width: parent.width
        height: parent.height

        Rectangle {
            width: parent.width
            height: parent.height
            radius: parent.delegateProperties.radius
            color: parent.delegateProperties.shadow.color
            opacity: parent.delegateProperties.shadow.opacity
        }

        Text {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 10
            color: "white"
            text: "Custom shadow"
        }
    }

    // Define the style itself, and tell it to use the custom delegates to render
    // the controls (instead of the otherwise default StyledItem):

    checkBox.checked.indicator.delegate: OverlayDelegate {}
    radioButton.checked.indicator.foreground.delegate: OverlayDelegate {}
    switchControl.handle.delegate: WavingQt {}

    abstractButton {
        background {
            delegate: UnderlayDelegate {}
            shadow.opacity: 0
            data: QtObject {
                // When using custom delegates, you might need pass properties to it that
                // should change per state. The 'data' property can be used for this purpose.
                property real underlayScale: 1.1
            }
        }
        hovered.background.data: QtObject {
            property real underlayScale: 1.4
        }
    }

    slider {
        background.visible: true
        background.delegate: NoiseDelegate {}
        indicator.delegate: NoiseDelegate {}
        handle.delegate: SliderHandle { isFirstHandle: true }
        handle.second.delegate: SliderHandle { isFirstHandle: false }
    }

    textField {
        background.shadow.color: "lightgray"
        background.shadow.verticalOffset: 14
        background.shadow.horizontalOffset: 14
        background.shadow.delegate: CustomShadowDelegate {}
        // verify that the delegate is allowed to change per state
        pressed.background.shadow.delegate: CustomShadowDelegate {}
        variations: StyleVariation {
            button.background {
                delegate: null
                implicitWidth: 30
                implicitHeight: 20
                margins: 10
                shadow.color: "transparent"
            }
        }
    }
}
