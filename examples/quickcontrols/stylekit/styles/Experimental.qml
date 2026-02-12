// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

Style {
    id: style

    control {
        leftPadding: 10
        topPadding: 5
        rightPadding: 10
        bottomPadding: 5

        background {
            implicitWidth: 100
            implicitHeight: 40
            shadow {
                opacity: 0.8
                scale: 1.1
                verticalOffset: 5
                horizontalOffset: 2
            }
        }

        handle {
            implicitWidth: 25
            implicitHeight: 25
            radius: 25
            color: "lightslategray"
        }

        indicator {
            foreground.margins: 2

            // first.color: "green"
            // second.color: "yellow"
            // up.color: "red" // alias. Can control.indicator.up have control.indicator as alt?
            // down.color: "blue"
        }

        text.color: "blue"

        transition: Transition {
            StyleAnimation {
                animateColors: true
                animateBackgroundBorder: true
                duration: 300
            }
        }

        hovered {
            transition: null
            background {
                border.width: 4
            }
            handle {
                border.width: 2
            }
            text.color: "yellow"
        }

        pressed {
            background {
                scale: 0.95
            }
        }
    }

    pane {
        background {
            border.width: 0
            implicitWidth: 200
            implicitHeight: 200
            shadow.visible: false
        }
    }

    frame {
        background {
            border.width: 1
            shadow.visible: true
        }
    }

    button {
        background {
            radius: 8
            opacity: 0.8

            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.alpha("black", 0.0)}
                GradientStop { position: 1.0; color: Qt.alpha("black", 0.4)}
            }
        }
    }

    radioButton {
        indicator {
            foreground {
                margins: 4
                radius: 25 / 2
                border.width: 0
            }
        }
    }

    textInput {
        background {
            implicitWidth: 200
        }
    }

    spinBox {
        padding: 0
        background {
            implicitWidth: 100
        }
    }

    comboBox {
        background {
            implicitWidth: 200
            implicitHeight: 30
            gradient: null
        }
        indicator {
            foreground.scale: 0.5
        }
        pressed.background.scale: 1.0
            variations: [contextMenu, mini, alert, "dont crash"]
        }

        StyleVariation {
            id: contextMenu
            itemDelegate {
                background {
                    radius: 0
                    border.width: 0
                    shadow.color: "transparent"
                    gradient: null
                }
                hovered.background.color: "pink"
            }

            control.transition: null
        }

        StyleVariation {
            id: mini
            control.background.implicitHeight: 10
        }

        StyleVariation {
            id: alert
            control.background.color: "red"
        }

    slider {
        background.implicitWidth: 180
        indicator {
            implicitHeight: 8
            radius: 8
            foreground {
                radius: 8
            }
        }
        handle {
            radius: 25
            first {
                topRightRadius: 0
                bottomRightRadius: 0
                color: "orange"
            }
            second {
                topLeftRadius: 0
                bottomLeftRadius: 0
                color: "yellow"
            }
        }
        vertical {
            handle {
                first {
                    radius: 25
                    topLeftRadius: 0
                    bottomLeftRadius: 0
                }
                second {
                    radius: 25
                    topRightRadius: 0
                    bottomRightRadius: 0
                }
            }
        }
    }

    switchControl {
        indicator {
            implicitWidth: 60
            implicitHeight: 30
            radius: 5
            foreground.radius: 4
        }
        handle {
            leftMargin: 3
            rightMargin: 3
        }
    }

    itemDelegate {
        background {
            radius: 0
            border.width: 0
            shadow.visible: false
            gradient: null
        }
        transition: null
        hovered {
            background.color: style.palette.highlight
        }
    }

    // THEMES

    light: Theme {
        control {
            background {
                border.color: "lightgray"
                shadow.color: "#808080"
            }

            checked {
                background.shadow.color: "white"
                background.color: "blue"
            }

            focused {
                background.border.color: "white"
                background.shadow.color: "white"
            }

            hovered {
                background {
                    border.color: "white"
                    shadow.color: "white"
                }
            }

            disabled {
                background {
                    color: "#a7a7a7"
                    shadow.color: "transparent"
                }
            }
        }

        button {
            background.color: "lightgray"
        }

        textField {
            variations: StyleVariation {
                button {
                    background {
                        radius: 0
                        implicitWidth: 50
                        implicitHeight: 20
                        margins: 4
                        color: "orange"
                        shadow.color: "transparent"
                    }
                }
            }
        }

        palettes {
            system.window: "#989898"
            textField.text: "#4e4e4e"
            button {
                buttonText: "white"
                highlightedText: "white"
                brightText: "#4e4e4e"
                disabled.buttonText: "#4e4e4e"
                disabled.highlightedText: "#4e4e4e"
            }
        }
    }

    dark: Theme {
        control {
            background {
                border.color: "#3d373b"
                shadow.color: "#404040"
                color: "#8e848a"
            }

            handle {
                border.color: "black"
            }

            focused {
                background {
                    border.color: "white"
                    shadow.color: "white"
                    color: "#bbbbbb"
                }
            }

            hovered {
                background {
                    border.color: "white"
                    shadow.color: "white"
                }
            }

            disabled {
                background {
                    color: "red"//"#766e73"
                    shadow.color: "transparent"
                }
                checked.background.color: "green"
            }
        }

        textField {
            variations: StyleVariation {
                button {
                    background {
                        radius: 0
                        implicitWidth: 50
                        implicitHeight: 20
                        margins: 4
                        color: "lightblue"
                        shadow.color: "transparent"
                    }
                }
            }
        }

        palettes {
            system.window: "#544e52"
            textField.text: "black"
            button {
                buttonText: "white"
                highlightedText: "white"
                brightText: "white"
                disabled.buttonText: "darkgray"
                disabled.highlightedText: "darkgray"
            }
        }
    }

    CustomTheme {
        name: "HighContrast"
        theme: Theme {
            control {
                background {
                    border.color: "black"
                    shadow.color: "transparent"
                    color: "white"
                    gradient: null
                }

                indicator {
                    implicitWidth: 30
                    implicitHeight: 30
                }

                handle {
                    border.color: "black"
                    implicitWidth: 30
                    implicitHeight: 30
                    radius: 30
                }

                hovered {
                    background {
                        border.color: "red"
                        border.width: 3
                    }
                    indicator {
                        border.color: "red"
                        border.width: 3
                    }
                }

                checked {
                    background {
                        border.color: "red"
                        border.width: 3
                    }
                }

                disabled {
                    background.color: "gray"
                }
            }

            slider {
                indicator {
                    implicitWidth: 180
                    implicitHeight: 8
                }
            }

            switchControl {
                spacing: 2
                indicator {
                    implicitWidth: 60
                    implicitHeight: 34
                }
            }

            palettes {
                system.window: "white"
                textField.text: "black"
                button {
                    buttonText: "black"
                    highlightedText: "black"
                    brightText: "black"
                    disabled.buttonText: "white"
                    disabled.highlightedText: "white"
                }
            }
        }
    }

    CustomTheme {
        name: "Green"
        theme: Theme {
            control {
                background {
                    border.color: "lightgray"
                    shadow.color: "#404040"
                    color: "#a0c0a0"
                }

                handle {
                    border.color: "lightgray"
                    shadow.color: "#404040"
                    color: "#a0c0a0"
                }

                indicator {
                    foreground.color: "lime"
                    foreground.image.color: "black"
                }

                hovered {
                    background {
                        border.color: "lightgreen"
                        shadow.color: "lightgreen"
                        color: "lightgreen"
                    }
                    handle {
                        border.color: "lightgreen"
                        shadow.color: "lightgreen"
                        color: "lightgreen"
                    }
                }

                checked {
                    background {
                        shadow.color: "lightgreen"
                        color: "lightgreen"
                    }
                }

                disabled {
                    background {
                        color: "#80a080"
                        shadow.color: "transparent"
                    }
                }
            }

            palettes {
                system.window: "#547454"
                textField.text: "green"
                checkBox.buttonText: "white"
                button {
                    buttonText: "black"
                    highlightedText: "white"
                    disabled.buttonText: "lightgray"
                    disabled.highlightedText: "lightgray"
                }
            }
        }
    }

    CustomTheme {
        name: "Empty"
        theme: Theme {}
    }
}
