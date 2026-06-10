// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

Style {
    id: fusionStyle

    component OverlayData : QtObject {
        property color overlayColor
    }

    component BackgroundDelegate: StyledItem {
        id: backgroundDelegate
        width: parent.width
        height: parent.height

        readonly property color backgroundColor: backgroundDelegate.delegateStyle.data
                                                 ? backgroundDelegate.delegateStyle.data.overlayColor : "transparent"
        property int radius: 2

        Rectangle {
            anchors.fill: parent
            color: backgroundDelegate.backgroundColor
            gradient: Gradient {
                GradientStop { position: 0.0; color: backgroundDelegate.backgroundColor }
                GradientStop { position: 1.0; color: Qt.lighter(backgroundDelegate.backgroundColor, 1.05) }
            }
            radius: backgroundDelegate.radius
            border.color: "#121111"
        }

        Rectangle {
            x: 1; y: 1
            width: parent.width - 2
            height: parent.height - 2
            color: "transparent"
            radius: backgroundDelegate.radius
            border.color: "#424040"
        }
    }

    component HandleDelegate: StyledItem {
        id: handleDelegate
        property int radius: 2

        width: parent.width
        height: parent.height

        Rectangle {
            anchors.fill: parent
            color: handleDelegate.delegateStyle.color
            gradient: Gradient {
                GradientStop { position: 0.0; color: handleDelegate.delegateStyle.color }
                GradientStop { position: 1.0; color: Qt.lighter(handleDelegate.delegateStyle.color, 1.05) }
            }
            border.width: 1
            border.color: "transparent"
            radius: handleDelegate.radius
        }

        Rectangle {
            width: parent.width
            height: parent.height
            radius: handleDelegate.radius
            color: "transparent"
            border.color: "#121111"

            Rectangle {
                x: 1; y: 1
                anchors.centerIn: parent
                width: parent.width - 2
                height: parent.height - 2
                border.color: "#424040"
                color: "transparent"
                radius: handleDelegate.radius
            }
        }
    }

    Gradient {
        id: indicatorGradient
        GradientStop { position: 0.0; color: Qt.lighter("darkgray", 1.25) }
        GradientStop { position: 1.0; color: Qt.lighter("darkgray", 1.4) }
    }

    Gradient {
        id: lightForegroundGradient
        GradientStop { position: 0.0; color: Qt.alpha("white", 0.2) }
        GradientStop { position: 1.0; color: Qt.alpha("white", 0.8) }
    }

    Gradient {
        id: darkForegroundGradient
        GradientStop { position: 0.0; color: Qt.alpha("black", 0.2) }
        GradientStop { position: 1.0; color: Qt.alpha("white", 0.1) }
    }

    control {
        leftPadding: 10
        rightPadding: 10

        background.border.width: 1
        handle {
            implicitWidth: 13
            implicitHeight: 13
            radius: 2
        }

        indicator {
            implicitWidth: 15
            implicitHeight: 15
            radius: 2
        }
    }

    abstractButton {
        background {
            implicitWidth: 80
            implicitHeight: 24
        }
    }

    comboBox {
        background {
            implicitWidth: 120
            implicitHeight: 24
        }
    }

    radioButton {
        indicator {
            radius: width / 2
            foreground.margins: 4
        }
    }

    roundButton {
        background.radius: 255
    }

    switchControl {
        indicator {
            implicitWidth: 40
            implicitHeight: 16
            foreground.radius: 1
        }
        handle {
            radius: 0
            implicitWidth: 20
            implicitHeight: 16
        }
    }

    scrollBar {
        background.visible: false
        padding: 2
        indicator {
            implicitWidth: 7
            implicitHeight: 7
        }
    }

    scrollIndicator {
        padding: 2
        indicator {
            implicitWidth: 7
            implicitHeight: 7
        }
    }

    slider {
        indicator {
            implicitWidth: 140
            implicitHeight: 5
            foreground {
                margins: 0
                border.width: 1
            }
        }
        vertical {
            background {
                implicitWidth: 5
                implicitHeight: 140
            }
            indicator {
                implicitWidth: 5
                implicitHeight: 140
            }
        }
    }

    spinBox {
        background {
            implicitWidth: 120
            implicitHeight: 24
        }
    }

    textField {
        background {
            implicitWidth: 120
            implicitHeight: 24
        }
    }

    popup {
        padding: 1
    }

    itemDelegate {
        background.border.width: 0
    }

    light: Theme {
        applicationWindow {
            background.color: "white"
        }

        control {
            background {
                color: Qt.lighter("whitesmoke", 1.03)
                border.color: "darkgray"
            }

            handle {
                color: "white"
            }

            indicator {
                color: "white"
            }

            hovered {
                background.color: "transparent"
            }

            pressed {
                indicator.color: "lightgray"
                handle.color: Qt.lighter("lightgray", 1.15)
            }
        }

        abstractButton {
            checked {
                background.color: Qt.lighter("lightgray", 1.05)
                hovered.background.color: Qt.lighter("lightgray", 1.1)
            }
            pressed.background.color: Qt.lighter("lightgray", 1.1)
            disabled {
                background {
                    color: "white"
                    border.color: Qt.darker("lightgray", 1.2)
                    opacity: 0.5
                }
                text.color: "lightgray"
            }
        }

        flatButton {
            hovered.background.visible: true
            checked.background.visible: true
        }

        comboBox {
            indicator.color: "transparent"
            pressed.background.color: Qt.lighter("lightgray", 1.1)
        }

        scrollBar {
            indicator {
                foreground.color: Qt.darker("lightgray", 1.05)
                border.color: Qt.darker("lightgray", 1.05)
            }
        }

        switchControl {
            indicator {
                color: Qt.lighter("darkgray", 1.05)
                border.color: Qt.lighter("darkgray", 1.05)
                gradient: indicatorGradient
            }
            checked {
                indicator {
                    border.color: palette.accent
                    foreground {
                        color: palette.accent
                        gradient: lightForegroundGradient
                    }
                }
            }
        }

        slider {
            indicator {
                color: Qt.lighter("darkgray", 1.05)
                border.color: Qt.lighter("darkgray", 1.05)
                gradient: indicatorGradient

                foreground {
                    border.color: palette.accent
                    color: palette.accent
                    gradient: lightForegroundGradient
                }
            }
        }

        textField {
            focused {
                background.border.color: palette.accent
            }
        }

        spinBox {
            focused {
                background.border.color: palette.accent
            }
        }

        itemDelegate {
            hovered.background.color: palette.accent
        }
    }

    dark: Theme {
        applicationWindow {
            background.color: "#1e1e1e"
        }

        control {
            text.color: "#fafafa"

            background {
                color: "#1e1e1e"
                border.color: "#121111"
            }

            indicator {
                color: "#1e1e1e"
                border.color: "#121111"
            }
        }


        abstractButton {
            background.delegate: BackgroundDelegate {}

            background.data: OverlayData {
                overlayColor: "#262525"
            }

            hovered.background.data: OverlayData {
                overlayColor: Qt.lighter("#262525", 1.15)
            }

            pressed.background.data: OverlayData {
                overlayColor: Qt.lighter("#1e1e1e", 1.15)
            }

            checked {
                background.data: OverlayData {
                    overlayColor: "#1e1e1e"
                }

                background {
                    color: "#1e1e1e"
                    gradient: null
                }

                hovered.background.data: OverlayData {
                    overlayColor: Qt.lighter("#1e1e1e", 1.1)
                }
            }

            disabled {
                background {
                    color: "#363636"
                    opacity: 0.5
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: Qt.alpha("white", 0.03) }
                        GradientStop { position: 1.0; color: Qt.alpha("black", 0.1) }
                    }
                }
                text.color: "#666666"
            }
        }

        flatButton {
            checked.background.visible: true
        }

        checkBox {
            pressed.indicator.color: "#363333"
        }

        comboBox {
            background.delegate: BackgroundDelegate {}
            background.data: OverlayData {
                overlayColor: "#262525"
            }

            hovered.background.data: OverlayData {
                overlayColor: Qt.lighter("#262525", 1.15)
            }

            pressed.background.data: OverlayData {
                overlayColor: "#1e1e1e"
            }
            indicator.color: "transparent"
        }

        radioButton {
            indicator {
                color: "#2E2B2B"
                border.color: "#121111"
            }
            pressed.indicator.color: "#363333"
        }

        roundButton {
            background.delegate: BackgroundDelegate { radius: height / 2 }
        }

        scrollBar {
            indicator {
                foreground.color: Qt.lighter("#121111", 1.5)
                border.color: Qt.lighter("#121111", 1.5)
            }
        }

        scrollIndicator {
            indicator {
                foreground.color: Qt.lighter("#121111", 1.5)
                border.color: Qt.lighter("#121111", 1.5)
            }
        }

        switchControl {
            handle {
                color: "#2E2B2B"
                delegate: HandleDelegate { radius: 0 }
            }

            checked {
                indicator {
                    border.color: palette.accent
                    color: palette.accent
                    gradient: darkForegroundGradient
                    foreground.visible: false
                }
            }

            hovered.handle.color: Qt.lighter("#2E2B2B", 1.1)
            pressed.handle.color: Qt.lighter("#1e1e1e", 1.15)
        }

        slider {
            handle {
                color: "#2E2B2B"
                delegate: HandleDelegate {}
            }

            indicator {
                foreground {
                    border.color: palette.accent
                    color: palette.accent
                    gradient: darkForegroundGradient
                }
            }

            hovered.handle.color: Qt.lighter("#2E2B2B", 1.1)
            pressed.handle.color: Qt.lighter("#1e1e1e", 1.15)
        }

        spinBox {
            background.delegate: BackgroundDelegate {}
            background.data: OverlayData {
                overlayColor: "#262525"
            }

            hovered.background.data: OverlayData {
                overlayColor: Qt.lighter("#262525", 1.15)
            }

            pressed.background.data: OverlayData {
                overlayColor: "#1e1e1e"
            }
            indicator.color: "transparent"
        }

        textField {
            background.color: Qt.lighter("#121111", 1.2)
            focused {
                background.border.color: palette.accent
            }
        }

        itemDelegate {
            hovered.background.color: palette.accent
        }
    }
}
