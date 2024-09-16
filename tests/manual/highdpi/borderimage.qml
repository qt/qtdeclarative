// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    width: 900
    height: 400

    readonly property real imageBorder: 32
    readonly property real animDuration: 3000
    readonly property real animMin: 2 * imageBorder
    readonly property real animMax: 280

    Text {
        anchors.bottom: row.top
        anchors.horizontalCenter: parent.horizontalCenter
        text: "Green => standard DPI; purple => @2x"
    }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 10
        Repeater {
            model: 3
            delegate: Item {
                width: animMax
                height: animMax
                BorderImage {
                    source : index === 0 ? "BorderImage.png" : "TiledBorderImage.png"
                    anchors.centerIn: parent

                    border {
                        left: imageBorder; right: imageBorder
                        top: imageBorder; bottom: imageBorder
                    }

                    horizontalTileMode: index === 0 ? BorderImage.Stretch :
                                        index === 1 ? BorderImage.Repeat : BorderImage.Round
                    verticalTileMode: index === 0 ? BorderImage.Stretch :
                                      index === 1 ? BorderImage.Repeat : BorderImage.Round

                    width: animMin
                    SequentialAnimation on width {
                        NumberAnimation { to: animMax; duration: animDuration }
                        NumberAnimation { to: animMin; duration: animDuration }
                        loops: Animation.Infinite
                    }

                    height: animMax
                    SequentialAnimation on height {
                        NumberAnimation { to: animMin; duration: animDuration }
                        NumberAnimation { to: animMax; duration: animDuration }
                        loops: Animation.Infinite
                    }
                }

                Text {
                    anchors.top: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.pixelSize: 18
                    text: index === 0 ? "Stretch" :
                          index === 1 ? "Repeat" : "Round"
                }
            }
        }
    }
}
