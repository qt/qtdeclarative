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

    //! [underlay]
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

        slider.handle.delegate: Item {
            required property DelegateStyle delegateStyle

            implicitWidth: delegateStyle.implicitWidth
            implicitHeight: delegateStyle.implicitHeight
            width: parent.width
            height: parent.height
            scale: delegateStyle.scale
            rotation: delegateStyle.rotation
            visible: delegateStyle.visible

            // Draw a star underneath the default handle delegate
            Star {
                width: parent.width * 2
                height: parent.height * 2
                anchors.centerIn: parent
                color: "gold"
            }

            StyledItem {
                delegateStyle: parent.delegateStyle
            }
        }
    }
    //! [underlay]

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
