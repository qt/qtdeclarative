// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [1]
// PlainStyle.qml

import QtQuick
import Qt.labs.StyleKit

Style {
    control {
        padding: 6
        background {
            radius: 4
            implicitWidth: 100
            implicitHeight: 36
        }
        indicator {
            implicitWidth: 20
            implicitHeight: 20
            border.width: 1
        }
        handle {
            implicitWidth: 20
            implicitHeight: 20
            radius: 10
        }
    }

    button {
        background {
            implicitWidth: 120
            shadow.opacity: 0.6
            shadow.verticalOffset: 2
            shadow.horizontalOffset: 2
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.alpha("black", 0.0)}
                GradientStop { position: 1.0; color: Qt.alpha("black", 0.2)}
            }
        }
        pressed.background.scale: 0.95
    }

    slider {
        indicator.implicitWidth: Style.Stretch
        indicator.implicitHeight: 6
        indicator.radius: 3
    }

    light: Theme {
        applicationWindow {
            background.color: "whitesmoke"
        }
        control {
            text.color: "black"
            background.color: "#e8e8e8"
            background.border.color: "#c0c0c0"
            hovered.background.color: "#d0d0d0"
        }
        button {
            text.color: "white"
            background.color: "cornflowerblue"
            background.shadow.color: "gray"
            hovered.background.color: "royalblue"
        }
    }

    dark: Theme {
        applicationWindow {
            background.color: Qt.darker("gray", 2.0)
        }
        control {
            text.color: "white"
            background.color: "#3a3a3a"
            background.border.color: "#555555"
            hovered.background.color: "#4a4a4a"
        }
        button {
            background.color: "sandybrown"
            background.shadow.color: "black"
            hovered.background.color: Qt.darker("sandybrown", 1.2)
        }
    }
}
//! [1]
