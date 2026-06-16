// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

Style {
    id: style

    control {
        // 'control' is the fallback for all the controls. Any properties that are not
        // overridden by a specific control underneath will be read from here instead.
        leftPadding: 8
        topPadding: 2
        rightPadding: 8
        bottomPadding: 2

        handle {
            implicitWidth: 25
            implicitHeight: 25
            radius: 25
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.alpha("black", 0.0)}
                GradientStop { position: 1.0; color: Qt.alpha("black", 0.2)}
            }
        }

        indicator {
            foreground.margins: 2
        }

        transition: Transition {
            StyleAnimation {
                animateColors: true
                duration: 300
            }
        }

        hovered {
            transition: null
        }
    }

    abstractButton {
        // 'abstractButton' is the fallback for all button types such as 'button', 'checkBox',
        // 'radioButton', etc. This is a good place to style the properties they all
        // have in common. Any properties not set here will fall back to those defined in 'control'.
        background {
            implicitWidth: 100
            implicitHeight: 30
            opacity: 0.8
            radius: 8

            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.alpha("black", 0.0)}
                GradientStop { position: 1.0; color: Qt.alpha("black", 0.2)}
            }
        }
    }

    flatButton {
        background.gradient: null
    }

    checkBox {
        transition: Transition {
            NumberAnimation {
                properties: "indicator.foreground.leftMargin, indicator.foreground.rightMargin"
                            + ", indicator.foreground.topMargin, indicator.foreground.bottomMargin"
                easing.type: Easing.OutBounce
                duration: 500
            }
        }
        hovered {
            transition: null
            indicator.foreground.margins: -15
        }
    }

    comboBox {
        background.implicitWidth: 200
    }

    pane {
        // 'pane' is the fallback for all pane based controls, such as 'frame' and 'groupBox'.
        //  Any properties not set here will fall back to those defined in 'control'.
        spacing: 5
        padding: 20
        background {
            border.width: 0
            implicitWidth: 200
            implicitHeight: 200
        }
    }

    groupBox {
        background.topMargin: 20
        background.implicitHeight: 30
        background.border.width: 1
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

    scrollBar {
        padding: 2
        background.visible: false
        indicator.implicitWidth: 8
        indicator.implicitHeight: 8
        indicator.radius: 8
        indicator.foreground.radius: 8
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
        vertical {
            background.implicitWidth: 40
            background.implicitHeight: 180
            indicator.implicitWidth: 8
        }
    }

    spinBox {
        padding: 4
        background.implicitWidth: 100
        indicator.implicitHeight: 24
    }

    textInput {
        // 'textInput' is the fallback for all text based controls, such as 'textField', 'textArea'.
        // Any properties not set here will fall back to those defined in 'control'.
        background {
            implicitWidth: 200
        }
    }

    // A style can have any number of themes. The ones assigned to 'light' and 'dark'
    // will be applied according to the current system theme if 'themeName' is set to
    // "System" (the default). Setting the current themeName for a style is usually done
    // from the application rather than from within the style itself.
    //
    // Within a theme, you can override any properties that should have different values
    // when the theme is applied. Typically, a style configures structural properties
    // such as implicit size, padding, and radii, while a theme specifies colors. However,
    // this is not a limitation — any properties can be overridden by a theme. Properties
    // not set in the theme will fall back to those defined in the style.

    light: Theme {
        applicationWindow {
            background.color: palette.window
        }

        control {
            background {
                color: "lightgray"
                border.color: "white"
            }

            handle {
                color: "lightgray"
                border.color: "white"
            }

            indicator {
                color: "white"
                foreground.image.color: palette.accent
            }

            checked {
                background.color: "blue"
            }

            focused {
                background.border.color: "white"
            }

            hovered {
                background {
                    color: palette.accent
                    border.color: "white"
                }
                handle {
                    border.color: "lightgray"
                }
            }

            disabled {
                background {
                    opacity: 0.4
                    gradient: null
                }
            }
        }

        abstractButton {
            hovered.background {
                color: palette.accent
            }
            checked {
                background.color: palette.accent
            }
        }

        pane {
            background.color: Qt.darker("gainsboro", 1.05)
        }

        groupBox {
            background.border.color: "white"
        }

        textField {
            text.color: palette.text
            background {
                border.color: "darkgray"
                color: "white"
            }
            hovered.background {
                border.color: "lightgray"
            }
            focused {
                background.border.color: palette.accent
                background.border.width: 2
            }
            focused.hovered {
                background.border.color: palette.accent
            }
        }
    }

    dark: Theme {
        applicationWindow {
            background.color: "#544e52"
        }

        control {
            text.color: "lightgray"
            background {
                border.color: "#3d373b"
                color: "#8e848a"
            }

            handle {
                color: "#8e848a"
                border.color: Qt.darker("#544e52", 1.5)
            }

            indicator {
                color: Qt.darker("#8e848a", 1.6)
            }

            hovered {
                background {
                    border.color: "white"
                    color: palette.accent
                }
            }
        }

        abstractButton {
            checked {
                background.color: palette.accent
            }

            disabled {
                background {
                    opacity: 0.3
                }
                checked.background.color: "green"
            }

            focused {
                background {
                    border.color: "white"
                    color: "#bbbbbb"
                }
            }
        }

        textInput {
            background.color: "white"
        }

        scrollBar {
            indicator.foreground.color: "white"
        }

        slider {
            indicator.foreground.color: palette.accent
        }

        pane {
            background.color: Qt.lighter("#544e52", 1.3)
            background.border.color: "#3d373b"
        }

        palettes {
            system.window: "#544e52"
            textField.text: "black"
            spinBox.highlight: "blue"
            button {
                buttonText: "white"
                highlightedText: "white"
                brightText: "white"
                disabled.buttonText: "darkgray"
                disabled.highlightedText: "darkgray"
            }
        }
    }

    // In addition to 'light' and 'dark', you can define as many themes as you want.
    // You can switch between them from the application, for example using:
    // 'StyleKit.style.themeName: "HighContrast"'.

    CustomTheme {
        name: "HighContrast"
        theme: Theme {
            control {
                transition: null

                background {
                    implicitHeight: 40
                    color: "lightgray"
                    border.color: "black"
                    border.width: 2
                    gradient: null
                }

                indicator {
                    implicitWidth: 30
                    implicitHeight: 30
                    color: "ghostwhite"
                    border.color: "black"
                    foreground.margins: 4
                    foreground.color: "black"
                    foreground.image.color: "ghostwhite"
                }

                handle {
                    border.color: "black"
                    border.width: 2
                    implicitWidth: 30
                    implicitHeight: 30
                    radius: 30
                    gradient: null
                }

                hovered {
                    background.border.width: 4
                    indicator.border.width: 4
                    handle.border.width: 4
                }

                checked {
                    background.border.width: 6
                }

                disabled {
                    background.color: "white"
                }
            }

            abstractButton {
                background.color: "ghostwhite"
            }

            textInput {
                background.color: "white"
            }

            slider {
                indicator {
                    implicitWidth: 180
                    implicitHeight: 12
                    color: "ghostwhite"
                    border.width: 1
                    foreground.color: "black"
                }
                vertical {
                    indicator.implicitWidth: 12
                    indicator.implicitHeight: 180
                }
            }

            radioButton {
                indicator.radius: 255
                indicator.foreground.radius: 255
            }

            spinBox {
                indicator.color: "black"
                indicator.foreground.image.color: "white"
                hovered.background.border.width: 6
            }

            itemDelegate {
                background.border.width: 0
                hovered.background.border.width: 2
                hovered.text.bold: false
            }

            scrollBar {
                background.implicitWidth: 15
                background.implicitHeight: 15
                indicator.implicitWidth: 15
                indicator.implicitHeight: 15
                indicator.border.width: 3
                indicator.foreground.margins: 3
                indicator.foreground.color: "lightgray"
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
            applicationWindow {
                background.color: "#8da28d"
            }

            control {
                background {
                    border.color: "#547454"
                    color: "#a0c0a0"
                }

                handle {
                    border.color: "#547454"
                    color: "#a0c0a0"
                }

                indicator {
                    color: "white"
                    border.color: "#547454"
                    foreground.color: "white"
                }

                text {
                    color: "#1c261c"
                    bold: true
                }

                hovered {
                    background {
                        color: "#ecefec"
                        border.color: "#ecefec"
                    }
                    handle {
                        color: "#ecefec"
                        border.color: "#ecefec"
                    }
                }

                checked {
                    indicator {
                        foreground.color: "#678367"
                        foreground.image.color: "#678367"
                    }
                }

                disabled {
                    background {
                        color: "#80a080"
                    }
                }
            }

            checkBox {
                indicator.foreground.color: "transparent"
            }

            comboBox {
                indicator.color: "transparent"
                indicator.foreground.color: "transparent"
                indicator.foreground.image.color: "white"
            }

            pane {
                background.color: "#a0b1a0"
                background.border.color: "#415a41"
            }

            scrollIndicator {
                indicator.foreground.color: "white"
            }

            spinBox {
                indicator.color: "transparent"
                indicator.foreground.color: "transparent"
                indicator.foreground.image.color: "white"
            }

            textInput {
                background.color: "white"
            }

            palettes {
                system.window: "#547454"
                textField.text: "green"
                textField.placeholderText: "#678367"
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

}
