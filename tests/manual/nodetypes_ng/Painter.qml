// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0
import Stuff 1.0

Item {
    ListModel {
        id: balloonModel
        ListElement {
            balloonWidth: 200
        }
        ListElement {
            balloonWidth: 120
        }
        ListElement {
            balloonWidth: 120
        }
        ListElement {
            balloonWidth: 120
        }
        ListElement {
            balloonWidth: 120
        }
    }

    ListView {
        anchors.fill: parent
        anchors.margins: 10
        id: balloonView
        model: balloonModel
        spacing: 5
        delegate: TextBalloon {
            anchors.right: index % 2 == 0 ? undefined : parent.right
            height: 60
            rightAligned: index % 2 == 0 ? false : true
            width: balloonWidth
            innerAnim: model.index === 1
            NumberAnimation on width {
                from: 200
                to: 300
                duration: 5000
                running: model.index === 0
            }
        }
    }
}
