// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//![0]
Component {
    id: listViewDelegate
    Rectangle {
        width: 100
        height: 50

        ListView.onPooled: rotationAnimation.pause()
        ListView.onReused: rotationAnimation.resume()

        Rectangle {
            id: rect
            anchors.centerIn: parent
            width: 40
            height: 5
            color: "green"

            RotationAnimation {
                id: rotationAnimation
                target: rect
                duration: (Math.random() * 2000) + 200
                from: 0
                to: 359
                running: true
                loops: Animation.Infinite
            }
        }
    }
}
//![0]
