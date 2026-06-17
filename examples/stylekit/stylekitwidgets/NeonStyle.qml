// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

Style {
    id: style

    control {
        leftPadding: 12
        topPadding: 4
        rightPadding: 12
        bottomPadding: 4

        background {
            radius: 2
            border.width: 2
        }

        handle {
            width: 20
            height: 20
            radius: 2
            border.width: 2
        }

        indicator {
            border.width: 2
            foreground.margins: 3
        }

        transition: Transition {
            StyleAnimation {
                animateColors: true
                duration: 200
            }
        }
    }

    abstractButton {
        background {
            width: 110
            height: 32
        }
    }

    flatButton {
        background.border.width: 0
    }

    checkBox {
        indicator {
            width: 20
            height: 20
        }
    }

    radioButton {
        indicator {
            width: 20
            height: 20
            radius: 10
            foreground.radius: 10
        }
    }

    slider {
        background.width: 180
        indicator {
            height: 6
            radius: 0
            foreground {
                radius: 0
                margins: 0
            }
        }
        vertical {
            background.height: 180
            background.width: 40
            indicator.width: 6
        }
    }

    spinBox {
        padding: 4
        background.width: 110
    }

    textInput {
        background {
            width: 220
            radius: 0
        }
    }

    pane {
        padding: 14
        background {
            radius: 2
            border.width: 2
        }
    }

    scrollBar {
        background.visible: false
    }

    dark: Theme {
        applicationWindow.background.color: "#0a0a14"

        control {
            text.color: "#00ffd0"
            background {
                color: "#14142a"
                border.color: "#00ffd0"
            }
            handle {
                color: "#1a1a35"
                border.color: "#ff2bd6"
            }
            indicator {
                color: "#14142a"
                border.color: "#00ffd0"
                foreground.color: "#00ffd0"
                foreground.image.color: "#0a0a14"
            }
            checked {
                background.color: "#00ffd0"
                indicator.color: "#00ffd0"
            }
            hovered {
                background.border.color: "#ff2bd6"
            }
            focused {
                background.border.color: "#ffe600"
            }
            disabled {
                background {
                    opacity: 0.3
                }
            }
        }

        abstractButton {
            background.color: "#1a1a35"
            checked.background.color: "#00ffd0"
            hovered.background.color: "#28284f"
        }

        textInput {
            background.color: "#14142a"
            focused.background.border.color: "#ffe600"
        }

        slider {
            indicator.color: "#303080"
            indicator.foreground.color: "#00ffd0"
        }

        switchControl {
            indicator {
                color: "#1a1a35"
                foreground.color: "transparent"
            }
            checked.indicator.color: "#00ffd0"
        }

        pane {
            background.color: "#14142a"
            background.border.color: "#ff2bd6"
        }
    }

    light: Theme {
        applicationWindow.background.color: "#f0f0ff"

        control {
            text.color: "#3a2bd6"
            background {
                color: "#ffffff"
                border.color: "#3a2bd6"
            }
            handle {
                color: "#ffffff"
                border.color: "#ff2bd6"
            }
            indicator {
                color: "#ffffff"
                border.color: "#3a2bd6"
                foreground.color: "#3a2bd6"
                foreground.image.color: "#ffffff"
            }
            checked {
                background.color: "#3a2bd6"
                indicator.color: "#3a2bd6"
            }
            hovered {
                background.border.color: "#ff2bd6"
            }
            focused {
                background.border.color: "#00b894"
            }
            disabled.background.opacity: 0.4
        }

        abstractButton {
            background.color: "#e6e0ff"
            checked.background.color: "#3a2bd6"
            hovered.background.color: "#d0c4ff"
        }

        textInput {
            background.color: "#ffffff"
            focused.background.border.color: "#00b894"
        }

        slider {
            indicator.color: "#e6e0ff"
            indicator.foreground.color: "#3a2bd6"
        }

        switchControl {
            indicator {
                color: "#e6e0ff"
                foreground.color: "transparent"
            }
            checked.indicator.color: "#3a2bd6"
        }

        pane {
            background.color: "#ffffff"
            background.border.color: "#ff2bd6"
        }

        palettes {
            system.window: "#f0f0ff"
            textField.text: "#3a2bd6"
            button {
                buttonText: "#3a2bd6"
                highlightedText: "#ffffff"
                brightText: "#00b894"
                disabled.buttonText: "#aaaacc"
            }
        }
    }
}
