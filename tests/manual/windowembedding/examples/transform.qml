// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    id: rootItem
    gradient: Gradient.TemptingAzure
    width: 500; height: 500

    Rectangle {
        id: rectangle
        width: 200; height: 200
        color: "lightgray"

        transformOrigin: Item.TopLeft
        transform: [
            Translate { id: translation; x: 50; y: 50 }
        ]
        scale: 1.5

        WindowContainer {
            width: 200; height: 200
            window: Window {
                color: "lightgray"

                Image {
                    source: "https://placedog.net/500/500?random"
                    anchors.fill: parent
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        rectangle.scale += 0.1
                    }
                }
            }
        }
    }
}
