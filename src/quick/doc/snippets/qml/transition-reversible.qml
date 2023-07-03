// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

//![0]
Rectangle {
    id: rect
    width: 100; height: 100
    color: "steelblue"

    TapHandler { id: tapHandler }

    states: State {
        name: "brighter"
        when: tapHandler.pressed
        PropertyChanges { target: rect; color: "lightsteelblue"; x: 50 }
    }

    //! [sequential animations]
    transitions: Transition {
        to: "brighter"
        reversible: true
        SequentialAnimation {
            PropertyAnimation { property: "x"; duration: 1000 }
            ColorAnimation { duration: 1000 }
        }
    }
    //! [sequential animations]
}
//![0]

