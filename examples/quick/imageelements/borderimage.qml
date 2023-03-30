// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: page

    width: 320
    height: 480

    BorderImageSelector {
        id: selector

        curIdx: 0
        maxIdx: 3
        gridWidth: 240
        flickable: mainFlickable
        width: parent.width
        height: 64
    }

    Flickable {
        id: mainFlickable

        width: parent.width
        anchors.bottom: parent.bottom
        anchors.top: selector.bottom
        interactive: false //Animated through selector control
        contentX: -120
        Behavior on contentX { NumberAnimation {}}
        contentWidth: 1030
        contentHeight: 420
        Grid {
            anchors.centerIn: parent
            spacing: 20

            MyBorderImage {
                minWidth: 120
                maxWidth: 240
                minHeight: 120
                maxHeight: 200
                source: Qt.resolvedUrl("pics/colors.png")
                margin: 30
            }

            MyBorderImage {
                minWidth: 120
                maxWidth: 240
                minHeight: 120
                maxHeight: 200
                source: Qt.resolvedUrl("pics/colors.png")
                margin: 30
                horizontalMode: BorderImage.Repeat
                verticalMode: BorderImage.Repeat
            }

            MyBorderImage {
                minWidth: 120
                maxWidth: 240
                minHeight: 120
                maxHeight: 200
                source: Qt.resolvedUrl("pics/colors.png")
                margin: 30
                horizontalMode: BorderImage.Stretch
                verticalMode: BorderImage.Repeat
            }

            MyBorderImage {
                minWidth: 120
                maxWidth: 240
                minHeight: 120
                maxHeight: 200
                source: Qt.resolvedUrl("pics/colors.png")
                margin: 30
                horizontalMode: BorderImage.Round
                verticalMode: BorderImage.Round
            }

            MyBorderImage {
                minWidth: 60
                maxWidth: 200
                minHeight: 40
                maxHeight: 200
                source: Qt.resolvedUrl("pics/bw.png")
                margin: 10
            }

            MyBorderImage {
                minWidth: 60
                maxWidth: 200
                minHeight: 40
                maxHeight: 200
                source: Qt.resolvedUrl("pics/bw.png")
                margin: 10
                horizontalMode: BorderImage.Repeat
                verticalMode: BorderImage.Repeat
            }

            MyBorderImage {
                minWidth: 60
                maxWidth: 200
                minHeight: 40
                maxHeight: 200
                source: Qt.resolvedUrl("pics/bw.png")
                margin: 10
                horizontalMode: BorderImage.Stretch
                verticalMode: BorderImage.Repeat
            }

            MyBorderImage {
                minWidth: 60
                maxWidth: 200
                minHeight: 40
                maxHeight: 200
                source: Qt.resolvedUrl("pics/bw.png")
                margin: 10
                horizontalMode: BorderImage.Round
                verticalMode: BorderImage.Round
            }
        }
    }
}
