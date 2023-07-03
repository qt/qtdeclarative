// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    width: 160; height: 250

    Image {
        width: 160; height: 200
        source: "pics/checker.svg"
        fillMode: Image.Tile

        //! [colors]
        Rectangle {
            color: "steelblue"
            width: 40; height: 40
        }
        Rectangle {
            color: "transparent"
            y: 40; width: 40; height: 40
        }
        Rectangle {
            color: "#FF0000"
            y: 80; width: 40; height: 40
        }
        Rectangle {
            color: "#800000FF"
            y: 120; width: 40; height: 40
        }
        Rectangle {
            color: "#00000000"    // ARGB fully transparent
            y: 160
            width: 40; height: 40
        }
        //! [colors]

        Rectangle {
            x: 40
            width: 120; height: 200

            Text {
                font.pixelSize: 16
                text: "steelblue"
                x: 10; height: 40
                verticalAlignment: Text.AlignVCenter
            }
            Text {
                font.pixelSize: 16
                text: "transparent"
                x: 10; y: 40; height: 40
                verticalAlignment: Text.AlignVCenter
            }
            Text {
                font.pixelSize: 16
                text: "FF0000"
                x: 10; y: 80; height: 40
                verticalAlignment: Text.AlignVCenter
            }
            Text {
                font.pixelSize: 16
                text: "800000FF"
                x: 10; y: 120; height: 40
                verticalAlignment: Text.AlignVCenter
            }
            Text {
                font.pixelSize: 16
                text: "00000000"
                x: 10; y: 160; height: 40
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    Image {
        y: 210
        width: 40; height: 40
        source: "pics/checker.svg"
        fillMode: Image.Tile
    }

    Text {
        font.pixelSize: 16
        text: "(background)"
        x: 50; y: 210; height: 40
        verticalAlignment: Text.AlignVCenter
    }
}
