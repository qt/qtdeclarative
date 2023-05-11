// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import WearableStyle

Rectangle {
    color: UIStyle.themeColorQtGray8

    Row {
        spacing: 5
        width: parent.width - 80
        anchors.centerIn: parent

        Image {
            id: img
            anchors.verticalCenter: parent.verticalCenter
            source: navImage
            fillMode: Image.PreserveAspectFit
        }

        Column {
            spacing: 5
            width: parent.width - img.width
            anchors.verticalCenter: parent.verticalCenter

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: navInstruction
                font.pixelSize: UIStyle.fontSizeS
                verticalAlignment: Text.AlignVCenter
                padding: 1
                color: UIStyle.themeColorQtGray1
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: navAuxInfo
                font.pixelSize: UIStyle.fontSizeXS
                verticalAlignment: Text.AlignVCenter
                padding: 1
                color: UIStyle.themeColorQtGray2
            }
        }
    }
}
