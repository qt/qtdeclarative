// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias popup: popup
    property alias popup2: popup2
    property alias popup3: popup3
    property alias button: button
    property alias slider: slider

    Button {
        id: button
        text: "Open"
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -height

        Popup {
            id: popup
            y: parent.height

            Text {
                color: "white"
                text: "Hello, world!"

                MouseArea {
                    anchors.fill: parent
                    onClicked: popup.close()
                }
            }
        }

        Popup {
            id: popup3
            y: parent.height

            Slider {
                id: slider
            }
        }
    }

    Popup {
        id: popup2
        y: popup.y
        z: 1
        contentItem: Text {
            text: "Popup2"
            font.pixelSize: 36
        }
    }
}
