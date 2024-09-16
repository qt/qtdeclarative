// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    id: rootItem
    gradient: Gradient.TemptingAzure
    width: 500; height: 500

    property int windowZ: 0

    WindowContainer {
        id: redWindow
        window: Window {
            color: "red"

            MouseArea {
                anchors.fill: parent
                onClicked: redWindow.z = ++rootItem.windowZ
            }
        }
    }

    WindowContainer {
        id: greenWindow
        x: 100; y: 100
        window: Window {
            color: "green"

            MouseArea {
                anchors.fill: parent
                onClicked: greenWindow.z = ++rootItem.windowZ
            }
        }
    }

    WindowContainer {
        id: blueWindow
        x: 200; y: 200
        window: Window {
            color: "blue"

            MouseArea {
                anchors.fill: parent
                onClicked: blueWindow.z = ++rootItem.windowZ
            }
        }
    }
}
