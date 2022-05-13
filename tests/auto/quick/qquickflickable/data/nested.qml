// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick 2.12

Flickable {
    id: root
    objectName: "root flickable"
    width: 240; height: 240
    contentHeight: 1000

    Flickable {
        flickableDirection: Flickable.HorizontalFlick
        width: root.width
        height: 100
        contentWidth: 1000
        objectName: "inner flickable"
        Repeater {
            model: 10
            Rectangle {
                x: 100 * index
                width: 96
                height: 96
                color: Qt.rgba(Math.random(), Math.random(), Math.random(), Math.random())
            }
        }
    }
}
