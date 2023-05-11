// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls as QQC2
import WearableStyle
import "notifications.js" as NotificationData

ListView {
    id: missedCallsView

    clip: true
    focus: true
    boundsBehavior: Flickable.StopAtBounds
    snapMode: ListView.SnapToItem

    model: ListModel {
        id: missedCallsList
    }

    Image {
        id: missedCallIcon
        width: parent.width / 2
        anchors.right: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        source: UIStyle.themeImagePath("notifications-missedcall")
        fillMode: Image.Pad
    }

    delegate: Item {
        height: missedCallsView.height
        width: missedCallsView.width / 2
        anchors.left: missedCallsView.contentItem.horizontalCenter

        Column {
            spacing: 15
            anchors.verticalCenter: parent.verticalCenter

            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                source: UIStyle.themeImagePath(`notifications-avatar${model.gender}`)
            }

            Text {
                text: model.name
                anchors.horizontalCenter: parent.horizontalCenter
                font.bold: true
                font.pixelSize: UIStyle.fontSizeS
                color: UIStyle.themeColorQtGray1
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: date + " " + time
                font.pixelSize: UIStyle.fontSizeXS
                font.italic: true
                color: UIStyle.themeColorQtGray2
            }
        }
    }

    Component.onCompleted: {
        NotificationData.populateData(missedCallsList)
    }
}
