// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.12

ListView {
    id: listView
    property var lastItem
    anchors.fill: parent
    model: 10
    delegate: Rectangle {
        width: listView.width
        height: 40
        border.color: "lightsteelblue"
        Text {
            text: "Item" + (index + 1)
        }
        Component.onCompleted: {
            if (index == 9)
                listView.lastItem = this
        }
    }

    populate: Transition {
        NumberAnimation {
            properties: "x,y"
            duration: 1000
        }
    }
}
