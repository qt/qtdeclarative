// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import WearableStyle

ListHeaderItem {
    id: routeListItem
    required property string icon
    required property string shortInfo
    required property string instruction
    required property string distance

    Item {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 28

        Image {
            id: naviIcon
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.margins: 5
            height: 20
            width: height
            source: UIStyle.iconPath(routeListItem.icon)
            fillMode: Image.PreserveAspectFit
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: naviIcon.right
            anchors.margins: 5
            wrapMode: Text.WordWrap
            text: routeListItem.shortInfo
            font: UIStyle.h3
            verticalAlignment: Text.AlignVCenter
            padding: 1
            color: UIStyle.textColor
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.margins: 10
            wrapMode: Text.WordWrap
            text: routeListItem.distance
            font: UIStyle.h3
            verticalAlignment: Text.AlignVCenter
            padding: 1
            color: UIStyle.textColor
        }
    }

    Text {
        id: instructionText
        anchors.fill: parent
        anchors.margins: 5
        anchors.topMargin: 25
        wrapMode: Text.WordWrap
        text: routeListItem.instruction
        font: UIStyle.p1
        lineHeight: UIStyle.p1lineHeight
        lineHeightMode: Text.FixedHeight
        verticalAlignment: Text.AlignVCenter
        padding: 1
        color: UIStyle.textColor
    }
}
