// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [file]
import QtQuick
import QtQuick.Controls.Basic

ProgressBar {
    id: control
    value: 0.5
    padding: 2

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 6
        color: "#e6e6e6"
        radius: 3
    }

    contentItem: Item {
        implicitWidth: 200
        implicitHeight: 4

        // Progress indicator for determinate state.
        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: 2
            color: "#17a81a"
            visible: !control.indeterminate
        }

        // Scrolling animation for indeterminate state.
        Item {
            anchors.fill: parent
            visible: control.indeterminate
            clip: true

            Row {
                spacing: 20

                Repeater {
                    model: control.width / 40 + 1

                    Rectangle {
                        color: "#17a81a"
                        width: 20
                        height: control.height
                    }
                }
                XAnimator on x {
                    from: 0
                    to: -40
                    loops: Animation.Infinite
                    running: control.indeterminate
                }
            }
        }
    }
}
//! [file]
