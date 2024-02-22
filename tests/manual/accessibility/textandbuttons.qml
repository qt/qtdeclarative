// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    id : rect
    width: 300
    height: 200

    Rectangle {
        width : 200
        height : 20

        id: button
        anchors.top : rect.top
        anchors.topMargin: 30
        property string text : "Click to activate"
        property int counter : 0

        Accessible.role : Accessible.Button

        Accessible.onPressAction: {
            buttonAction()
        }

        function buttonAction() {
            ++counter
            text = "clicked " + counter

            text2.x += 20
        }

        Text {
            id : text1
            anchors.fill: parent
            text : parent.text
        }

        MouseArea {
            id : mouseArea
            anchors.fill: parent
            onClicked: parent.buttonAction()
        }
    }

    Text {
        id : text2
        anchors.top: button.bottom
        anchors.topMargin: 50
        text : "Hello World " + x

        Behavior on x { PropertyAnimation { duration: 500 } }
    }
}
