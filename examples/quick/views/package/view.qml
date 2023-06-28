// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQml.Models

pragma ComponentBehavior: Bound

Rectangle {
    id: root
    color: "white"
    width: 320
    height: 480
    property int upTo
    SequentialAnimation on upTo {
        loops: -1
        NumberAnimation {
            to: 8
            duration: 3500
        }
        NumberAnimation {
            to: 0
            duration: 3500
        }
    }

    ListModel {
        id: myModel
        ListElement { display: "One" }
        ListElement { display: "Two" }
        ListElement { display: "Three" }
        ListElement { display: "Four" }
        ListElement { display: "Five" }
        ListElement { display: "Six" }
        ListElement { display: "Seven" }
        ListElement { display: "Eight" }
    }
    //![0]
    DelegateModel {
        id: visualModel
        delegate: Delegate {
            upTo: root.upTo
        }
        model: myModel
    }

    ListView {
        id: lv
        height: parent.height / 2
        width: parent.width

        model: visualModel.parts.list
    }
    GridView {
        y: parent.height / 2
        height: parent.height / 2
        width: parent.width
        cellWidth: width / 2
        cellHeight: 50
        model: visualModel.parts.grid
    }
    //![0]
    Text {
        anchors.bottom: parent.bottom
    }
}
