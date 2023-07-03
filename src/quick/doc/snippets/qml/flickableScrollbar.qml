// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//![0]
Rectangle {
    width: 200; height: 200

    Flickable {
        id: flickable
//![0]
        anchors.fill: parent
        contentWidth: image.width; contentHeight: image.height

        Image { id: image; source: "pics/qt.png" }
//![1]
    }

    Rectangle {
        id: scrollbar
        anchors.right: flickable.right
        y: flickable.visibleArea.yPosition * flickable.height
        width: 10
        height: flickable.visibleArea.heightRatio * flickable.height
        color: "black"
    }
}
//![1]
