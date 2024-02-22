// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick 2.9

Rectangle {
    width: 200
    height: 200
    ListView {
        id: list
        objectName: "list"
        width: 100
        height: 100
        model: 20
        anchors.centerIn: parent
        orientation: initialOrientation
        contentWidth: initialContentWidth
        contentHeight: initialContentHeight
        flickableDirection: initialFlickableDirection
        pixelAligned: true
        delegate: Rectangle {
            width: list.orientation == ListView.Vertical ? 120 : 10
            height: list.orientation == ListView.Vertical ? 20 : 110
            color: Qt.rgba(0, 0, index / 19, 1)
            opacity: 0.8
        }
        Rectangle {
            z: -1
            width: 100
            height: 100
            border.width: 1
            border.color: "red"
        }
    }
}
