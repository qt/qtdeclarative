// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
Rectangle {
    id: rootRect
    width: 100
    states: [
        State {
            name: "state1"
            PropertyChanges {
                target: rootRect
                width: 200
            }
        }
    ]
    transitions: [
        Transition {
            from: "*"
            to: "state1"
            PropertyAnimation {
                target: rootRect
                property: "width"
                duration: 100
            }
        }
    ]
}
