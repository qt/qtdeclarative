// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2

Rectangle {
    width: 200
    height: 200
    color: "white"

    property alias left_animator: left_animator
    property alias right_animator: right_animator
    property alias rectangle: rect

    Rectangle {
        id: rect
        width: 100
        height: 200
        color: "red"

        state: "left"
        states: [
            State {
                name: "left"
            },
            State {
                name: "right"
            }
        ]

        transitions: [
            Transition {
                to: "left"

                XAnimator {
                    id: left_animator
                    target: rect
                    duration: 500
                    to: 0
                }
            },
            Transition {
                to: "right"

                XAnimator {
                    id: right_animator
                    target: rect
                    duration: 500
                    to: 100
                }
            }
        ]
    }
}
