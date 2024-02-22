// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.1
import QtTest 1.1

Item {
    width: 300
    height: 542

    function resizeThirdItem(size) {
        resizingListModel.setProperty(3, "size", size)
    }

    ListView {
        id: list
        anchors.fill: parent
        model: ListModel {
            id: resizingListModel
            ListElement { size: 300; }
            ListElement { size: 300; }
            ListElement { size: 300; }
            ListElement { size: 300; }
            ListElement { size: 300; }
            ListElement { size: 300; }
        }
        delegate: Rectangle {
            width: list.width
            color: index % 2 == 0 ? "red" : "blue"
            height: size
            Text { anchors.centerIn: parent; text: index }
        }
    }

    Text { text: list.contentY; color: "white" }
}
