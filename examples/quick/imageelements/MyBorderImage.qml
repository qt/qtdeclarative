// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQml
import QtQuick

Item {
    id: container

    property alias horizontalMode: image.horizontalTileMode
    property alias verticalMode: image.verticalTileMode
    property alias source: image.source

    property int minWidth
    property int minHeight
    property int maxWidth
    property int maxHeight
    property int margin

    width: 240
    height: 200

    BorderImage {
        id: image

        anchors.centerIn: parent

        SequentialAnimation on width {
            loops: Animation.Infinite
            NumberAnimation {
                from: container.minWidth
                to: container.maxWidth
                duration: 2000
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                from: container.maxWidth
                to: container.minWidth
                duration: 2000
                easing.type: Easing.InOutQuad
            }
        }

        SequentialAnimation on height {
            loops: Animation.Infinite
            NumberAnimation {
                from: container.minHeight
                to: container.maxHeight
                duration: 2000
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                from: container.maxHeight
                to: container.minHeight
                duration: 2000
                easing.type: Easing.InOutQuad
            }
        }

        border.top: container.margin
        border.left: container.margin
        border.bottom: container.margin
        border.right: container.margin
    }
}
