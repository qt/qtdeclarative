// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Row {
    id: root
    width: 320
    height: 480
    Flickable {
        id: fl1
        objectName: "fl1"
        width: parent.width / 2
        height: parent.height
        contentHeight: 640
        Rectangle {
            width: fl1.width
            height: 640
            color: fl1.dragging ? "steelblue" : "lightsteelblue"
            Text {
                anchors.centerIn: parent
                text: "flick this"
            }
        }
    }
    Flickable {
        id: fl2
        objectName: "fl2"
        width: parent.width / 2
        height: parent.height
        contentHeight: 640
        Rectangle {
            width: fl2.width
            height: 640
            color: fl2.dragging ? "bisque" : "beige"
            Text {
                anchors.centerIn: parent
                text: "and flick\nthis too"
            }
        }
    }
}
