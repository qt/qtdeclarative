// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

//! [1]
ListView {
    id: listView
    anchors.fill: parent
    model: ListModel {
        ListElement { sender: "Bob Bobbleton"; title: "How are you going?" }
        ListElement { sender: "Rug Emporium"; title: "SALE! All rugs MUST go!" }
        ListElement { sender: "Electric Co."; title: "Electricity bill 15/07/2016 overdue" }
        ListElement { sender: "Tips"; title: "Five ways this tip will save your life" }
    }
    delegate: SwipeDelegate {
        id: swipeDelegate
        text: sender + " - " + title
        width: listView.width

        required property string sender
        required property string title
        required property int index

        ListView.onRemove: SequentialAnimation {
            PropertyAction {
                target: swipeDelegate
                property: "ListView.delayRemove"
                value: true
            }
            NumberAnimation {
                target: swipeDelegate
                property: "height"
                to: 0
                easing.type: Easing.InOutQuad
            }
            PropertyAction {
                target: swipeDelegate
                property: "ListView.delayRemove"
                value: false
            }
        }

        swipe.right: Label {
            id: deleteLabel
            text: qsTr("Delete")
            color: "white"
            verticalAlignment: Label.AlignVCenter
            padding: 12
            height: parent.height
            anchors.right: parent.right

            SwipeDelegate.onClicked: listView.model.remove(index)

            background: Rectangle {
                color: deleteLabel.SwipeDelegate.pressed ? Qt.darker("tomato", 1.1) : "tomato"
            }
        }
    }
}
//! [1]
