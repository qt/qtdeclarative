// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

Style {
    id: style

    fonts {
        system {
            family: "Courier New"
            pointSize: 12
        }
        textField.bold: true
        label.bold: true
    }

    control {
        // 'control' is the fallback for all the controls. Any properties that are not
        // overridden by a specific control underneath will be read from here instead.
        leftPadding: 10
        topPadding: 5
        rightPadding: 10
        bottomPadding: 5

        handle {
            implicitWidth: 25
            implicitHeight: 25
            radius: 25
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.alpha("black", 0.0)}
                GradientStop { position: 1.0; color: Qt.alpha("black", 0.2)}
            }
            shadow {
                opacity: 0.8
                scale: 1.1
            }
        }

        indicator {
            foreground.margins: 2
        }

        transition: Transition {
            StyleAnimation {
                animateColors: true
                animateBackgroundShadow: true
                animateHandleShadow: true
                duration: 300
            }
        }

        hovered {
            // For this style, we don't want to show any transitions when entering or while inside
            // the 'hovered' state. This makes the control light up immediately when hovered, but
            // fade out more slowly when returning to the 'normal' state. We therefore override
            // 'transition' and set it to null.
            transition: null
        }
    }

    abstractButton {
        // 'abstractButton' is the fallback for all button types such as 'button', 'checkBox',
        // 'radioButton', 'switch', etc. This is a good place to style the properties they all
        // have in common. Any properties not set here will fall back to those defined in 'control'.
        background {
            implicitWidth: 100
            implicitHeight: 40
            opacity: 0.8
            radius: 8

            shadow {
                opacity: 0.8
                scale: 1.1
            }

            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.alpha("black", 0.0)}
                GradientStop { position: 1.0; color: Qt.alpha("black", 0.2)}
            }
        }
    }

    button {
        // Here you can override the style for a Button. The properties you set here apply only
        // to a Button, not to for example a CheckBox. Any properties not set here will fall back
        // to those defined in 'abstractButton'.
        pressed {
            background.scale: 0.95
        }
    }

    checkBox {
        transition: Transition {
            NumberAnimation {
                // Using a StyleAnimation for transitions is optional. A StyleAnimation can be
                // used in parallel with other animations, or not used at all. Here we choose to use
                // a NumberAnimation instead to animate the 'checked' image so that it bounces.
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
        pressed.background.scale: 1.0
    }

    groupBox {
        background.topMargin: 30
        background.implicitHeight: 30
        text.bold: true
        spacing: 5
        padding: 10
    }

    pane {
        // 'pane' is the fallback for all pane based controls, such as 'frame' and 'groupBox'.
        //  Any properties not set here will fall back to those defined in 'control'.
        background {
            border.width: 0
            implicitWidth: 200
            implicitHeight: 200
            shadow.visible: false
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

    scrollBar {
        padding: 2
        background.visible: false
    }

    scrollIndicator {
        padding: 0
        indicator.foreground.margins: 0
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
    }

    spinBox {
        padding: 4
        background {
            implicitWidth: 100
            scale: 1
        }
        indicator.implicitHeight: 24
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

    textInput {
        // 'textInput' is the fallback for all text based controls, such as 'textField', 'textArea',
        // and 'searchField'. Any properties not set here will fall back to those defined in 'control'.
        background {
            implicitWidth: 200
        }
    }

    // You can define one or more StyleVariations that can be enabled from the
    // application using the attached 'StyleVariation.variations' property.
    // Inside a variation, you list the controls that should receive alternative
    // styling when the variation is active. Any properties defined in a variation
    // override those set in the Style or Theme.
    //
    // For example, if you set "StyleVariation.variations: ['mini']" on a GroupBox
    // in the application, all controls inside that GroupBox will be affected.
    // Exactly which controls are impacted depends on which ones you style inside
    // the variation.
    StyleVariation {
        name: "mini"

        control {
            padding: 2
            background {
                implicitHeight: 15
            }
            indicator {
                implicitWidth: 15
                implicitHeight: 15
            }
            handle {
                implicitWidth: 15
                implicitHeight: 15
            }
        }

        textInput {
            background.implicitWidth: 100
        }

        abstractButton.background {
            implicitWidth: 60
        }

        switchControl {
            background.implicitWidth: 40
            indicator.implicitWidth: 40
            indicator.implicitHeight: 20
        }

        slider {
            background.implicitWidth: 100
            indicator.implicitHeight: 8
            indicator.implicitWidth: Style.Stretch
        }
    }

    StyleVariation {
        name: "alert"
        abstractButton.background {
            color: "orchid"
            border.color: "orchid"
            shadow.color: "orchid"
        }
    }

    /* You can also set one or more StyleVariations on a control type. Unlike Instance
     * variations—which apply only to specific control instances—type variations are applied
     * to *all* instances of a control type without requiring the application to use attached
     * properties.
     *
     * In this example, we specify that all Buttons that are children of a Frame
     * should receive alternative styling, differentiating them from other Buttons. */
    frame {
        background {
            border.width: 1
            shadow.visible: true
        }
        variations: StyleVariation {
            button.background {
                radius: 0
                color: palette.accent
                shadow.visible: false
            }
        }
    }
    /* Because 'groupBox' falls back to 'frame', any StyleVariation applied to a frame
     * is automatically inherited by a groupBox as well. Since I in this example only want the
     * different styling on frames, not group boxes, I can simply unset the variation
     * for group boxes. */
    groupBox.variations: []

    readonly property int fancyButton: 0
    CustomControl {
        // You can also define your own custom control types and pair them with custom
        // implementations in your app. Here we provide a base configuration for a control
        // named 'fancyButton', and then override it in the themes to apply colors. Any
        // properties not set here will fall back to those defined in 'control'.
        // The 'controlType' can be any number between 0 and 100000.
        controlType: fancyButton
        background {
            implicitWidth: 200
            radius: 4
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
            background.color: "gainsboro"
        }

        control {
            background {
                color: "lightgray"
                border.color: "white"
                shadow.color: "white"
            }

            handle {
                color: "lightgray"
                shadow.color: "white"
                border.color: "white"
            }

            indicator {
                color: "white"
                foreground.image.color: palette.accent
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
                    color: palette.accent
                    border.color: "white"
                    shadow.color: "white"
                }
                handle {
                    shadow.color: "white"
                    shadow.scale: 1.6
                    border.color: "lightgray"
                }
            }

            disabled {
                background {
                    opacity: 0.4
                    shadow.visible: false
                    gradient: null
                }
            }
        }

        abstractButton {
            hovered.background {
                shadow.scale: 1.4
                color: palette.accent
            }
            checked {
                background.color: palette.accent
            }
        }

        pane {
            background.color: Qt.darker("gainsboro", 1.05)
        }

        switchControl {
            indicator.foreground.color: "white"
            checked.indicator.foreground.color: palette.accent
        }

        textField {
            background {
                shadow.scale: 0
                border.color: "darkgray"
                color: "white"
            }
            hovered.background {
                border.color: "lightgray"
                shadow.scale: 1.1
            }
            focused {
                background.border.color: palette.accent
                background.border.width: 2
            }
            focused.hovered {
                background.border.color: palette.accent
            }
        }

        CustomControl {
            controlType: fancyButton
            background {
                color: "lightslategray"
            }
        }

        // In a theme, you can also configure the theme palettes. These palettes act as
        // the base palettes for the entire application. The theme palettes in Qt Quick
        // are a combination of colors fetched from the operating system (including the
        // currently active OS theme) and any colors you override in the 'palettes'
        // section below.
        //
        // Because of this, StyleKit styles do not bind colors to the palette by default
        // (except for the accent color). Otherwise, the style’s appearance would vary
        // across platforms, since each platform defines its own palette—unless you
        // explicitly override all palette colors here. If you do want palette-based
        // behavior, you can bind properties to palette colors, e.g.:
        // 'button.text.color: palette.textColor'.

        palettes {
            system.window: "gainsboro"
            textField.text: "#4e4e4e"
            spinBox.highlight: "lightgray"

            button {
                buttonText: "black"
                highlightedText: "black"
                brightText: "#4e4e4e"
                disabled.buttonText: "#4e4e4e"
                disabled.highlightedText: "#4e4e4e"
            }
        }
    }

    dark: Theme {
        applicationWindow {
            background.color: "#544e52"
        }

        control {
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

            hovered {
                background {
                    border.color: "white"
                    color: palette.accent
                    shadow.color: "white"
                    shadow.scale: 1.1
                    shadow.blur: 20
                }

                handle {
                    shadow.color: "white"
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
                    shadow.color: "transparent"
                }
                checked.background.color: "green"
            }

            focused {
                background {
                    border.color: "white"
                    shadow.color: "white"
                    color: "#bbbbbb"
                }
            }
        }

        textInput {
            background.color: "white"
        }

        scrollBar {
            background.color: "#8e848a"
            indicator.foreground.color: "white"
        }

        switchControl {
            indicator.foreground.color: Qt.lighter("#8e848a", 1.3)
            checked.indicator.foreground.color: palette.accent
        }

        slider {
            indicator.foreground.color: palette.accent
        }

        pane {
            /* The controls change background color on states like hover, but panes
             * should not. Override the property here to disable that behavior for panes. */
            background.color: Qt.lighter("#544e52", 1.3)
            background.border.color: "#3d373b"
            background.shadow.visible: false
        }

        CustomControl {
            controlType: fancyButton
            background {
                color: "lightsteelblue"
            }
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
                    shadow.color: "transparent"
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

                text.bold: true

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
            }

            radioButton {
                indicator.radius: 255
                indicator.foreground.radius: 255
            }

            switchControl {
                background {
                    color: "ghostwhite"
                    border.width: 2
                }

                indicator {
                    radius: 16
                    margins: 0
                    border.width: 2
                    implicitWidth: 60
                    implicitHeight: 40
                    foreground.color: "transparent"
                }

                handle {
                    implicitWidth: 20
                    implicitHeight: 30
                    border.width: 2
                    color: "white"
                    margins: 6
                    radius: 0
                    topLeftRadius: 18
                    bottomLeftRadius: 18
                }

                hovered {
                    indicator.border.width: 4
                }

                checked {
                    handle {
                        color: "black"
                        radius: 0
                        topRightRadius: 18
                        bottomRightRadius: 18
                    }
                }
            }

            spinBox {
                indicator.color: "black"
                indicator.foreground.image.color: "white"
                hovered.background.border.width: 6
            }

            itemDelegate {
                background.border.width: 0
                hovered.background.border.width: 2
                hovered.text.bold: true
            }

            scrollBar {
                background.implicitWidth: 15
                background.implicitHeight: 15
                indicator.implicitWidth: 15
                indicator.implicitHeight: 15
                background.color: "#8e848a"
                background.border.width: 1
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
                    shadow.color: "darkseagreen"
                    color: "#a0c0a0"
                }

                handle {
                    border.color: "#547454"
                    shadow.color: "darkseagreen"
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
                        shadow.color: "white"
                    }
                    handle {
                        color: "#ecefec"
                        border.color: "#ecefec"
                        shadow.color: "white"
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
                        shadow.color: "transparent"
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
                background.shadow.visible: false
            }

            scrollIndicator {
                indicator.foreground.color: "white"
            }

            spinBox {
                indicator.color: "transparent"
                indicator.foreground.color: "transparent"
                indicator.foreground.image.color: "white"
            }

            switchControl {
                indicator.foreground.color: "white"
                checked.indicator.foreground.color: "#678367"
            }

            textInput {
                background.color: "white"
            }

            StyleVariation {
                name: "alert"
                abstractButton.background {
                    color: "lime"
                    border.color: "lime"
                    shadow.color: "lime"
                }
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

    CustomTheme {
        name: "Empty Theme"
        theme: Theme {}
    }
}
