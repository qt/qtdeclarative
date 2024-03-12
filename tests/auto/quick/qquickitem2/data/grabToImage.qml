// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Item {
    width: 320
    height: 480
    Rectangle {
        objectName: "myItem";
        width: 100
        height: 100
        color: "red"
        Rectangle {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: 10
            height: 10
            color: "blue"
        }
    }
}
