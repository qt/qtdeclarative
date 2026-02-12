// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

Style {
    id: style

    component NoiseDelegate : ShaderEffect {
        id: noiseDelegate
        implicitWidth: unifiedSourceItem.implicitWidth
        implicitHeight: unifiedSourceItem.implicitHeight
        width: parent.width
        height: parent.height
        scale: delegateProperties.scale
        rotation: delegateProperties.rotation
        visible: delegateProperties.visible

        required property StyleKitDelegateProperties delegateProperties

        readonly property bool isDarkBg: {
            let bgColor = delegateProperties.color
            let luminance = (0.2126 * bgColor.r) +  (0.7152 * bgColor.g) +  (0.0722 * bgColor.b);
            return luminance < 0.5;
        }

        // The following properties are used by the shader
        property size sourceItemSize: Qt.size(unifiedSourceItem.width, unifiedSourceItem.height)
        property color borderColor: delegateProperties.border.color
        property real borderMaskEnabled: 1
        property real borderMaskThreshold: 0.001
        property real particleDensity: 0.2
        property real particleSize: 0.5
        property color particleColor: "black"
        property Item source: ShaderEffectSource { live: true; sourceItem: unifiedSourceItem }
        property real time: 0
        property real particleOpacity: (delegateProperties.opacity === 1
                                       ? (isDarkBg ? 0.15 : 0.05)
                                       : (isDarkBg ? 0.5 : 0.1))

        fragmentShader: "qrc:/effects/noise.qsb"

        StyledItem {
            id: unifiedSourceItem
            delegateProperties: noiseDelegate.delegateProperties
            width: parent.width
            height: parent.height
            visible: false
            rotation: 0.0
            scale: 1.0
        }
    }

    component ColorSet : QtObject {
        property color normal
        property color muted
        property color subtle
    }

    property MyTheme myTheme: theme as MyTheme

    component MyTheme : Theme {
        property ColorSet neutralBackground
        property ColorSet neutralStroke
        property ColorSet neutralForeground
        property ColorSet accentBackground
        property ColorSet accentStroke
        property ColorSet accentForeground // on-accent

        property real controlHeight: 50
        property color windowColor: "#f7f7f7"
        property color shadowColor: "#404040"

        palettes {
            system.window: windowColor
            textField {
                text: neutralForeground.normal
                disabled.text: neutralForeground.muted
            }

            checkBox.buttonText: neutralForeground.normal
            button {
                buttonText: neutralForeground.normal
                highlightedText: neutralForeground.normal
                brightText: neutralForeground.normal //????
                disabled.buttonText: neutralForeground.subtle
                disabled.highlightedText: neutralForeground.subtle
            }
        }
    }

    Gradient {
        id: faintHorizontalGradient
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: Qt.alpha("black", 0.0)}
        GradientStop { position: 1.0; color: Qt.alpha("black", 0.1)}
    }

    Gradient {
        id: faintVerticalGradient
        orientation: Gradient.Vertical
        GradientStop { position: 0.0; color: Qt.alpha("black", 0.0)}
        GradientStop { position: 1.0; color: Qt.alpha("black", 0.1)}
    }

    Gradient {
        id: strongHorizontalGradient
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: Qt.alpha("black", 0.0)}
        GradientStop { position: 1.0; color: Qt.alpha("black", 0.4)}
    }

    Gradient {
        id: strongVerticalGradient
        orientation: Gradient.Vertical
        GradientStop { position: 0.0; color: Qt.alpha("black", 0.0)}
        GradientStop { position: 1.0; color: Qt.alpha("black", 0.4)}
    }

    applicationWindow {
        background.color: myTheme.windowColor
    }

    control {
        leftPadding: 10
        topPadding: 5
        rightPadding: 10
        bottomPadding: 5

        background {
            implicitHeight: myTheme.controlHeight
            color: myTheme.neutralBackground.normal
            delegate: NoiseDelegate {}
        }

        vertical {
            background.gradient: faintHorizontalGradient
            indicator.gradient: faintHorizontalGradient
        }

        handle {
            implicitWidth: 36
            implicitHeight: 36
            radius: 18
            border.width: 2
            border.color: myTheme.neutralStroke.normal
            color: myTheme.accentForeground.normal
            delegate: NoiseDelegate {}
        }

        indicator {
            implicitHeight: myTheme.controlHeight
            radius: 25
            border.width: 3
            border.color: myTheme.neutralStroke.normal
            color: myTheme.neutralBackground.normal
            delegate: NoiseDelegate {}
            foreground {
                radius: 25
                gradient: strongVerticalGradient
                border.width: 3
                border.color: myTheme.accentStroke.normal
                color: myTheme.accentBackground.normal
                delegate: NoiseDelegate {}
            }
        }

        hovered {
            handle.border.width: 5
        }
    }

    abstractButton {
        background {
            implicitWidth: 100
            radius: 255
            border.width: 2
            border.color: myTheme.neutralStroke.normal
            color: myTheme.neutralBackground.normal
            gradient: faintVerticalGradient
            delegate: NoiseDelegate {}

            shadow {
                opacity: 0.25
                scale: 1.05
                verticalOffset: 2.5
                horizontalOffset: 2
                color: myTheme.shadowColor
                blur: 5
            }
        }

        hovered {
            background.color: myTheme.neutralBackground.muted
            checked.background.color: myTheme.accentBackground.muted
        }

        pressed {
            background.scale: 0.95
        }

        checked {
            background {
                color: myTheme.accentBackground.normal
                border.color: myTheme.accentStroke.normal
            }
        }

        disabled {
            background {
                color: myTheme.neutralBackground.subtle
                border.color: myTheme.neutralStroke.subtle
                shadow.color: "transparent"
            }
        }
    }

    pane {
        background.delegate: null
    }

    scrollIndicator {
        padding: 2
        background.implicitHeight: 6
        indicator.implicitHeight: 6
        vertical {
            background.implicitWidth: 6
            indicator.implicitWidth: 6
        }
    }

    scrollBar {
        padding: 2
        background.implicitHeight: 20
        indicator.implicitHeight: 20
        vertical {
            background.implicitWidth: 20
            indicator.implicitWidth: 20
        }
    }

    checkBox {
        indicator {
            implicitWidth: 35
            implicitHeight: 35
            radius: 4
            border.width: 1.5
            foreground {
                radius: 4
                border.width: 1.5
                image.color: myTheme.accentForeground.normal
                color: myTheme.accentBackground.normal
            }
        }
    }

    radioButton {
        indicator {
            implicitWidth: 35
            implicitHeight: 35
            radius: width / 2
            border.width: 1.5
            foreground {
                margins: 4
                radius: width / 2
                border.width: 0
                //image.color: myTheme.activeHighlight
                color: myTheme.accentBackground.normal
                gradient: faintVerticalGradient
            }
        }
        checked {
            indicator.border.color: myTheme.accentStroke.normal
        }
    }

    popup {
        padding: 2
        topPadding: 20
        bottomPadding: 20
    }

    comboBox {
        background {
            implicitWidth: 200
            implicitHeight: myTheme.controlHeight
            radius: myTheme.controlHeight / 2
            border.color: myTheme.neutralStroke.normal
            color: myTheme.neutralBackground.normal
        }
        indicator {
            implicitHeight: myTheme.controlHeight / 6
            color: "transparent"
            border.width: 0
            foreground {
                margins: 4
                color: "transparent"
                border.width: 0
                gradient: null
                image.color: myTheme.neutralStroke.subtle
            }
        }
    }

    spinBox {
        background {
            radius: myTheme.controlHeight / 2
        }
        indicator {
            radius: 0
            color: "transparent"
            border.width: 0
            foreground.gradient: null
            foreground.color: "transparent"
            foreground.image.color: myTheme.accentStroke.normal
            foreground.border.width: 0
            foreground.implicitWidth: 20
            foreground.implicitHeight: 20
        }
    }

    textField {
        background {
            radius: 9999999999
            implicitWidth: 200
            implicitHeight: myTheme.controlHeight
            border.color: myTheme.neutralStroke.normal
            color: myTheme.neutralBackground.normal
        }
        hovered.background.color: myTheme.neutralBackground.muted
        focused.background.border.color: myTheme.accentStroke.normal
    }

    slider {
        spacing: 26
        background.implicitWidth: 180
        // indicator.implicitHeight: UnifiedStyle.Stretch
        indicator.foreground.minimumWidth: 50
        indicator.foreground.margins: 2
        indicator.foreground.delegate: null
        handle {
            leftMargin: 8
            rightMargin: 8
        }
    }

    switchControl {
        spacing: 8
        indicator {
            implicitWidth: 80
            implicitHeight: myTheme.controlHeight
            foreground.visible: false
        }
        handle {
            leftMargin: 8
            rightMargin: 8
        }
        checked {
            indicator {
                color: myTheme.accentBackground.normal
                border.color: myTheme.accentStroke.normal
                gradient: strongVerticalGradient
            }
        }
    }

    flatButton {
        hovered.background.visible: true
        checked.background.visible: true
        hovered.background.color: myTheme.neutralBackground.muted
        checked.background.color: myTheme.accentBackground.normal
        hovered.checked.background.color: myTheme.accentBackground.muted
    }

    itemDelegate {
        hovered.background.color: myTheme.accentBackground.normal
    }

    // THEMES

    light: MyTheme {
        windowColor: "#EFF5F5F5" // #F5F5F5 · 92% #EF
        shadowColor: "#AEAEAE"

        accentBackground: ColorSet {
            normal: "#8671EC"
            muted: "#B7ABF4"
            subtle: "#D9D2F9"
        }
        accentStroke: ColorSet {
            normal: "#4530B0"
            muted: "#654FD4"
            subtle: "#8671EC"
        }
        accentForeground: ColorSet {
            normal: "#FFFFFF"
            muted: "#909090"
            subtle: "#B7ABF4"
        }

        neutralBackground:  ColorSet {
            normal: "#FFFFFF" //#FFFFFF · 78% #C7
            muted: "#FCFCFC" //#FCFCFC · 92% #EF
            subtle: "#E3E3E3" //#E3E3E3 · 94% #F0
        }
        neutralStroke: ColorSet {
            normal: "#CDCDCD"
            muted: "#AEAEAE"
            subtle: "#BEBEBE"
        }
        neutralForeground: ColorSet {
            normal: "#000000"
            muted: "#2D2D2D"
            subtle: "#A9A9A9"
        }
    }

    dark: MyTheme {
        windowColor: "#EF444444" // #222222 · 92% #EF
        shadowColor: "#000"

        accentBackground: ColorSet {
            normal: "#654FD4"
            muted: "#4530B0"
            subtle: "#361EAB"
        }
        accentStroke: ColorSet {
            normal: "#654FD4"
            muted: "#654FD4"
            subtle: "#8671EC"
        }
        accentForeground: ColorSet {
            normal: "#E0E0E0"
            muted: "#A9A9A9"
            subtle: "#654FD4"
        }

        neutralBackground:  ColorSet {
            normal: "#434343" //#434343 · 78% #C7
            muted: "#636363" // #636363 · 92% #EF
            subtle: "#545454" //#545454 · 94% #F0
        }
        neutralStroke: ColorSet {
            normal: "#A9A9A9"
            muted: "#545454"
            subtle: "#3B3B3B"
        }
        neutralForeground: ColorSet {
            normal: "#FFFFFF"
            muted: "#BEBEBE"
            subtle: "#353535"
        }
    }

    CustomTheme {
        name: "Green"
        theme: MyTheme {
                windowColor: "#f0f4fbf4"

            accentBackground: ColorSet {
                normal: "green"
                muted: Qt.lighter("green")
                subtle: Qt.lighter("green", 2)
            }
            accentStroke: ColorSet {
                normal: "darkgreen"
                muted: Qt.lighter("darkgreen")
                subtle: Qt.lighter("darkgreen", 2)
            }
            accentForeground: ColorSet {
                normal: "#FFFFFF"
                muted: "#909090"
                subtle: "#B7ABF4"
            }

            neutralBackground:  ColorSet {
                normal: "#C7EEFFEE"
                muted: "#EFF0FCF0"
                subtle: "#F0E0F0E0"
            }
            neutralStroke: ColorSet {
                normal: "#CDDDCD"
                muted: "#AEBEAE"
                subtle: "#BECEBE"
            }
            neutralForeground: ColorSet {
                normal: "#000000"
                muted: "#2D2D2D"
                subtle: "#A9A9A9"
            }
        }
    }
}
