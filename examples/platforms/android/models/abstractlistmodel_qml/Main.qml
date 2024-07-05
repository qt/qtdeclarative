// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Controls

Rectangle {
    id: mainRectangle

    property AbstractItemModel dataModel

    color: "#00414A"
    border.width: 2
    border.color: "black"

    ListView {
        id: listView

        model: mainRectangle.dataModel
        ScrollBar.vertical: ScrollBar {}
        spacing: 10

        anchors {
            fill: parent
            margins: 20
        }

        delegate: Rectangle {
            required property var model

            width: listView.width
            height: textColumn.height + (2 * textColumn.spacing)
            color: "#2CDE85"
            radius: 25

            Column {
                id: textColumn

                height: idText.height + rowText.height + spacing
                spacing: 15

                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left
                    right: parent.right
                    leftMargin: 20
                    rightMargin: 20
                }

                Text {
                    id: idText

                    color: "#00414A"
                    text: model.id
                    font.pixelSize: 36
                    font.bold: true
                }

                Text {
                    id: rowText

                    color: "#00414A"
                    text: model.row
                    font.pixelSize: 36
                    font.bold: true
                }
            }
        }
    }
}
