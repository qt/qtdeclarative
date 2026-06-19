// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

Style {
    id: style

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
            height: myTheme.controlHeight
            color: myTheme.neutralBackground.normal
        }

        vertical {
            background.gradient: faintHorizontalGradient
            indicator.gradient: faintHorizontalGradient
        }

        handle {
            width: 36
            height: 36
            radius: 18
            border.width: 2
            border.color: myTheme.neutralStroke.normal
            color: myTheme.accentForeground.normal
        }

        indicator {
            height: myTheme.controlHeight
            radius: 25
            border.width: 3
            border.color: myTheme.neutralStroke.normal
            color: myTheme.neutralBackground.normal
            foreground {
                radius: 25
                gradient: strongVerticalGradient
                border.width: 3
                border.color: myTheme.accentStroke.normal
                color: myTheme.accentBackground.normal
            }
        }

        hovered {
            handle.border.width: 5
        }
    }

    abstractButton {
        background {
            width: 100
            radius: 255
            border.width: 2
            border.color: myTheme.neutralStroke.normal
            color: myTheme.neutralBackground.normal
            gradient: faintVerticalGradient

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
        padding: 20
        background.delegate: null
    }

    scrollIndicator {
        padding: 2
        background.height: 6
        indicator.height: 6
        vertical {
            background.width: 6
            indicator.width: 6
        }
    }

    scrollBar {
        padding: 2
        background.height: 20
        background.visible: false
        indicator.height: 20
        vertical {
            background.width: 20
            indicator.width: 20
        }
    }

    checkBox {
        indicator {
            width: 35
            height: 35
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
            width: 35
            height: 35
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

    roundButton {
        background.radius: 255
    }

    popup {
        padding: 2
        topPadding: 10
        bottomPadding: 10
    }

    comboBox {
        background {
            width: 200
            height: myTheme.controlHeight
            radius: myTheme.controlHeight / 2
            border.color: myTheme.neutralStroke.normal
            color: myTheme.neutralBackground.normal
        }
        indicator {
            height: myTheme.controlHeight / 6
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
            foreground.width: 20
            foreground.height: 20
        }
    }

    textField {
        background {
            radius: 9999999999
            width: 200
            height: myTheme.controlHeight
            border.color: myTheme.neutralStroke.normal
            color: myTheme.neutralBackground.normal
        }
        hovered.background.color: myTheme.neutralBackground.muted
        focused.background.border.color: myTheme.accentStroke.normal
    }

    slider {
        spacing: 26
        background.width: 180
        indicator.foreground.minimumWidth: 50
        indicator.foreground.margins: 2
        handle {
            leftMargin: 8
            rightMargin: 8
        }
        vertical {
            background {
                height: 180
                width: myTheme.controlHeight
            }
            indicator {
                width: myTheme.controlHeight
                foreground.minimumWidth: 0
                foreground.minimumHeight: 50
            }
            handle {
                leftMargin: 0
                rightMargin: 0
                topMargin: 8
                bottomMargin: 8
            }
        }
    }

    switchControl {
        spacing: 8
        indicator {
            width: 80
            height: myTheme.controlHeight
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

    menu {
        padding: 2
        topPadding: 10
        bottomPadding: 10
        background.width: 100
    }

    menuBar {
        padding: 2
        background.height: 30
    }

    menuBarItem {
        padding: 2
        background.width: 100
        background.height: 30
        hovered.background.color: myTheme.accentBackground.normal
    }

    menuItem {
        padding: 2
        text.padding: 4
        background.height: 30
        indicator.height: 30
        hovered.background.color: myTheme.accentBackground.normal
    }

    menuSeparator {
        padding: 4

        background {
            width: 100
            height: 0
            color: "transparent"
            border.width: 0
        }

        indicator {
            height: 1
            width: 100
            color: myTheme.neutralStroke.normal
            border.width: 0

            foreground.margins: 0
            foreground.visible: false
        }
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
