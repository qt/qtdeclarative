// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import TextBalloon

Item {
    height: 480
    width: 320

    //! [0]
    ListModel {
        id: balloonModel
        ListElement {
            balloonWidth: 200
        }
        ListElement {
            balloonWidth: 120
        }
    }

    ListView {
        id: balloonView
        anchors.bottom: controls.top
        anchors.bottomMargin: 2
        anchors.top: parent.top
        delegate: TextBalloon {
            anchors.right: index % 2 != 0 ? parent?.right : undefined
            height: 60
            rightAligned: index % 2 != 0
            width: balloonWidth
        }
        model: balloonModel
        spacing: 5
        width: parent.width
    }
    //! [0]

    //! [1]
    Rectangle {
        id: controls

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: 1
        anchors.right: parent.right
        border.width: 2
        color: "white"
        height: parent.height * 0.15

        Text {
            anchors.centerIn: parent
            text: qsTr("Add another balloon")
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                balloonModel.append({"balloonWidth": Math.floor(Math.random() * 200 + 100)})
                balloonView.positionViewAtIndex(balloonView.count -1, ListView.End)
            }
            onEntered: {
                parent.color = "#8ac953"
            }
            onExited: {
                parent.color = "white"
            }
        }
    }
    //! [1]
}
