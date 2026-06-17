// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

Style {
    id: style

    fonts {
        system {
            family: "Helvetica"
            pointSize: 11
        }
    }

    control {
        leftPadding: 10
        topPadding: 4
        rightPadding: 10
        bottomPadding: 4

        background {
            radius: 0
            border.width: 1
            gradient: null
        }

        handle {
            width: 18
            height: 18
            radius: 0
            border.width: 1
            gradient: null
        }

        indicator {
            border.width: 1
            foreground.margins: 3
        }

        transition: Transition {
            StyleAnimation {
                animateColors: true
                duration: 120
            }
        }
    }

    abstractButton {
        background {
            width: 100
            height: 28
        }
    }

    flatButton {
        background.border.width: 0
    }

    checkBox {
        indicator {
            width: 18
            height: 18
        }
    }

    radioButton {
        indicator {
            width: 18
            height: 18
            radius: 9
            foreground.radius: 9
        }
    }

    slider {
        background.width: 180
        indicator {
            height: 4
            radius: 0
            foreground {
                radius: 0
                margins: 0
            }
        }
        vertical {
            background.height: 180
            background.width: 40
            indicator.width: 4
        }
    }

    spinBox {
        padding: 4
        background.width: 100
    }

    textInput {
        background.width: 200
    }

    pane {
        padding: 12
        background {
            border.width: 1
        }
    }

    scrollBar {
        background.visible: false
    }

    light: Theme {
        applicationWindow.background.color: "#f5f5f5"

        control {
            text.color: "#222222"
            background {
                color: "#ffffff"
                border.color: "#cccccc"
            }
            handle {
                color: "#ffffff"
                border.color: "#999999"
            }
            indicator {
                color: "#ffffff"
                border.color: "#999999"
                foreground.color: "#1976d2"
                foreground.image.color: "#ffffff"
            }
            checked {
                background.color: "#1976d2"
                indicator.color: "#1976d2"
            }
            hovered {
                background.color: "#eeeeee"
                background.border.color: "#1976d2"
            }
            focused.background.border.color: "#1976d2"
            disabled.background.opacity: 0.4
        }

        abstractButton {
            checked.background.color: "#1976d2"
            hovered.background.color: "#e3f2fd"
        }

        textInput {
            background.color: "#ffffff"
            focused.background.border.color: "#1976d2"
        }

        slider {
            indicator.color: "#c8c8c8"
            indicator.foreground.color: "#1976d2"
        }

        switchControl {
            indicator {
                color: "#cccccc"
                foreground.color: "transparent"
            }
            checked.indicator.color: "#1976d2"
        }

        pane.background.color: "#ffffff"
    }

    dark: Theme {
        applicationWindow.background.color: "#1e1e1e"

        control {
            text.color: "#e0e0e0"
            background {
                color: "#2d2d2d"
                border.color: "#444444"
            }
            handle {
                color: "#3a3a3a"
                border.color: "#555555"
            }
            indicator {
                color: "#2d2d2d"
                border.color: "#666666"
                foreground.color: "#64b5f6"
                foreground.image.color: "#1e1e1e"
            }
            checked {
                background.color: "#64b5f6"
                indicator.color: "#64b5f6"
            }
            hovered {
                background.color: "#3a3a3a"
                background.border.color: "#64b5f6"
            }
            focused.background.border.color: "#64b5f6"
            disabled.background.opacity: 0.3
        }

        abstractButton {
            checked.background.color: "#64b5f6"
            hovered.background.color: "#3a3a3a"
        }

        textInput {
            background.color: "#2d2d2d"
            focused.background.border.color: "#64b5f6"
        }

        slider {
            indicator.color: "#4a4a4a"
            indicator.foreground.color: "#64b5f6"
        }

        switchControl {
            indicator {
                color: "#444444"
                foreground.color: "transparent"
            }
            checked.indicator.color: "#64b5f6"
        }

        pane.background.color: "#2d2d2d"

        palettes {
            system.window: "#1e1e1e"
            textField.text: "#e0e0e0"
            button {
                buttonText: "#e0e0e0"
                highlightedText: "#1e1e1e"
            }
        }
    }
}
