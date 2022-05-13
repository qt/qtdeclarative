// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtQuick.Window 2.0

Window {
    id: tw
    visible: true
    width: 800
    height: 500
    color: "green"
    Text {
        id: txt
        font.pointSize: 16
        anchors.top: parent.top
        text: "Move to the blue item.\nCheck the mouse cursor is a PointingHand.\nClick on the blue item."
    }

    Rectangle {
        anchors.centerIn: parent
        width: 100
        height: 50
        color: "blue"
        MouseArea {
            id: testHand
            anchors.fill: parent
            onClicked: {
                tw1.show()
            }
            cursorShape: Qt.PointingHandCursor
        }
    }

    Window {
        Text {
            font.pointSize: 16
            anchors.top: parent.top
            text: "Move the cursor to near one of the edges.\nClick the mouse button."
        }
        id: tw1
        visible: false
        width: 800
        height: 500
        color: "yellow"
        MouseArea {
            anchors.fill: parent
            onClicked: {
                tw1.close()
                txt.text = "Mouse cursor should now be back to an Arrow cursor"
            }
        }
    }
}
