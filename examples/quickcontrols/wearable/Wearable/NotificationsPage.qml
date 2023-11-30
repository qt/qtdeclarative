// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import WearableStyle
import "notifications.js" as NotificationData


Item {
    id: notificationpage

    ListView {
        id: listview

        anchors.fill: parent
        anchors.margins: 15
        anchors.topMargin: 40 + 15
        spacing: 10
        clip: false

        model: ListModel {
            id: missedCallsList
        }

        delegate: ListHeaderItem {
            id: notificationItem

            required property string name
            required property string date
            required property string time
            required property string text

            width: parent.width
            height: 80

            Item {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: 25
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.margins: 10
                    text: notificationItem.name
                    color: UIStyle.titletextColor
                    font: UIStyle.h3
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.margins: 10
                    text: notificationItem.date + " " + notificationItem.time
                    color: UIStyle.titletextColor
                    font: UIStyle.p1
                }
            }


            Text {
                anchors.fill: parent
                anchors.margins: 10
                anchors.topMargin: 30
                text: notificationItem.text
                wrapMode: Text.WordWrap
                color: UIStyle.textColor
                font: UIStyle.p1
                lineHeight: UIStyle.p1lineHeight
                lineHeightMode: Text.FixedHeight
            }
        }

        Component.onCompleted: {
            NotificationData.populateData(missedCallsList)
        }
    }
}
