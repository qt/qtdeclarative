// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

ApplicationWindow {
    id: app
    width: 1024
    height: 800
    visible: true

    StyleKit.style: Style {

        //! [transition]
        comboBox {
            background.color: "mistyrose"
            hovered.background.color: "plum"
            transition: Transition {
                StyleAnimation {
                    animateColors: true
                    animateBackgroundRadii: true
                    animateIndicatorRadii: true
                    animateBackgroundShadow: true
                    easing.type: Easing.OutQuad
                    duration: 500
                }
            }
        }
        //! [transition]

        //! [custom transition]
        button {
            background.color: "mistyrose"
            hovered.background.color: "plum"
            transition: Transition {
                ColorAnimation {
                    properties: "background.color, background.shadow.color, handle.color"
                    easing.type: Easing.OutQuad
                    duration: 500
                }
                NumberAnimation {
                    properties: "background.leftRadius, background.rightRadius"
                    easing.type: Easing.OutQuad
                    duration: 500
                }
            }

            // I only want a fade-out effect (not fade-in). So while the button
            // is hovered, remove the transition, so that it only applies in the
            // normal state. In other words, it's the state being entered that
            // determines the transition, not the state that is left.
            hovered.transition: null
        }
        //! [custom transition]
    }

    // The rest of the file is not a part of the docs. It just implements a small
    // UI to allow testing the style from the command line using the 'qml' app.

    ScrollView {
        anchors.fill: parent
        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            Button {
                text: "Button"
            }
        }
    }
}
