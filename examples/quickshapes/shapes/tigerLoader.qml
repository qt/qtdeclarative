// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    color: "lightGray"
    width: 256
    height: 256

    Item {
        anchors.fill: parent

        Text {
            anchors.centerIn: parent
            text: qsTr("Loading")
            // Phase #1: Loader loads tiger.qml. After this we have our item.
            // Phase #2: With some backends (generic) the item will start async processing. Wait for this too.
            visible: shapeLoader.status != Loader.Ready || (shapeLoader.item as Shape)?.status === Shape.Processing
        }

        Loader {
            id: shapeLoader
            anchors.fill: parent
            source: "tiger.qml"
            asynchronous: true
            visible: status == Loader.Ready
            scale: 0.4
        }
    }
}
