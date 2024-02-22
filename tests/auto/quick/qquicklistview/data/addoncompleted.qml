// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.9

Rectangle {
    width: 640
    height: 480
    color: "green"

    ListModel {
        id: listModel
        ListElement { name: "a" }
        ListElement { name: "b" }
        ListElement { name: "c" }
        ListElement { name: "d" }
        ListElement { name: "e" }
        ListElement { name: "f" }
        ListElement { name: "g" }
        ListElement { name: "h" }
        ListElement { name: "i" }
        ListElement { name: "j" }
    }

    ListView {
        anchors.fill: parent
        model: listModel
        objectName: "view"
        // buffered delegates are created asynchronously
        // therefore we disable buffering
        cacheBuffer: 0

        delegate: Rectangle {
            height: 15
            width: 15
            color: "blue"
            objectName: name
            Component.onCompleted: {
                if (name.length === 1 && listModel.get(index + 1).name.length === 1) {
                    for (var i = 0; i < 10; ++i)
                        listModel.insert(index + 1, {name: name + i});
                }
            }
        }
    }
}
