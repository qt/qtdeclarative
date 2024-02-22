// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Item {

    Rectangle {
        id: helpbutton; anchors { right: parent.right; bottom: parent.bottom }
        height: 30; width: 30; color: "lightgray"; radius: 5; visible: qmlfiletoload == ""
        Text { text: "?"; anchors.centerIn: parent; font.pointSize: 12 }
        MouseArea { anchors.fill: parent; onClicked: { elementsapp.qmlfiletoload = "Help.qml" } }
    }

    Rectangle {
        width: parent.width - (20 + helpbutton.width); height: infotext.height; radius: 5; opacity: .7; visible: infotext.text != ""
        anchors { right: helpbutton.left; bottom: parent.bottom; rightMargin: 5; bottomMargin: 20 }
        Text { id: infotext; text: elementsapp.helptext; width: parent.width - 10; anchors.centerIn: parent; wrapMode: Text.WordWrap }
    }
}
