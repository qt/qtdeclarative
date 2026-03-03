// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes
import Qt.labs.StyleKit

ApplicationWindow {
    id: app
    width: 1024
    height: 800
    visible: true

    StyleKit.style:

    //! [overlay]
    Style {
        component Star : Shape {
            id: star
            property color color
            ShapePath {
                fillColor: star.color
                scale: Qt.size(star.width, star.height)
                PathMove { x: 0.50; y: 0.00 }
                PathLine { x: 0.59; y: 0.35 }
                PathLine { x: 0.97; y: 0.35 }
                PathLine { x: 0.66; y: 0.57 }
                PathLine { x: 0.78; y: 0.91 }
                PathLine { x: 0.50; y: 0.70 }
                PathLine { x: 0.22; y: 0.91 }
                PathLine { x: 0.34; y: 0.57 }
                PathLine { x: 0.03; y: 0.35 }
                PathLine { x: 0.41; y: 0.35 }
                PathLine { x: 0.50; y: 0.00 }
            }
        }

        button {
            background.delegate: StyledItem {
                width: parent.width
                height: parent.height
                // Draw a star on top the default rendering
                Star {
                    anchors.fill: parent
                    color: "gold"
                }
            }
        }
    }
    //! [overlay]

    // The rest of the file is not a part of the docs. It just implements a small
    // UI to allow testing the style from the command line using the 'qml' app.

    GroupBox {
        title: "GroupBox"
        Column {
            spacing: 10

            Button {
                text: "Button"
            }

            Slider {
                width: 200
            }
        }
    }
}
