// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick 2.4

Rectangle {
    width: 240
    height: 320
    ListView {
        id: list
        objectName: "list"
        width: 100
        height: 100
        cacheBuffer: 0
        anchors.centerIn: parent
        model: testModel
        orientation: testOrientation
        layoutDirection: testLayoutDirection
        verticalLayoutDirection: testVerticalLayoutDirection
        delegate: Rectangle {
            width: 10
            height: 10
            border.width: 1
            border.color: "gray"
        }
        headerPositioning: ListView.PullBackHeader
        header: Rectangle {
            width: 10
            height: 10
            color: "red"
            objectName: "header"
        }
        footerPositioning: ListView.PullBackFooter
        footer: Rectangle {
            width: 10
            height: 10
            color: "blue"
            objectName: "footer"
        }
    }
}
