// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
//![0]
Rectangle {
    id: myRect
    width: 100; height: 100
    color: "red"

    MouseArea { id: mouseArea; anchors.fill: parent }

    states: State {
        name: "hidden"; when: mouseArea.pressed
        PropertyChanges { target: myRect; opacity: 0 }
    }
}
//![0]
