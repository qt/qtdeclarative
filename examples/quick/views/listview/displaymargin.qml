// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

Item {
    width: 480; height: 320

    ListView {
        id: view
        anchors.top: header.bottom
        anchors.bottom: footer.top
        width: parent.width

        cacheBuffer: 0
        displayMarginBeginning: 40
        displayMarginEnd: 40

        model: 100
        delegate: Rectangle {
            objectName: "delegate"
            width: parent.width
            height: 25
            color: index % 2 ? "steelblue" : "lightsteelblue"

            required property int index

            Text {
                anchors.centerIn: parent
                color: "white"
                text: "Item " + (parent.index + 1)
            }
        }
    }

    Rectangle {
        id: header
        width: parent.width
        height: 40
        color: "#AAFF0000"

        Text {
            anchors.centerIn: parent
            font.pixelSize: 24
            text: "Header"
        }
    }

    Rectangle {
        id: footer
        anchors.bottom: parent.bottom
        width: parent.width
        height: 40
        color: "#AAFF0000"

        Text {
            anchors.centerIn: parent
            font.pixelSize: 24
            text: "Footer"
        }
    }
}
