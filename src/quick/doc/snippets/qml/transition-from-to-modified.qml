// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

Rectangle {
    id: rect
    width: 100; height: 100
    color: "red"

    MouseArea { id: mouseArea; anchors.fill: parent }

    states: State {
        name: "brighter"; when: mouseArea.pressed
        PropertyChanges { target: rect; color: "yellow" }
    }

    //! [modified transition]
    transitions: Transition {
        to: "brighter"
        ColorAnimation { duration: 1000 }
    }
    //! [modified transition]
}
