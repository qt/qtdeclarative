// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.9

Flickable {
    width: 200; height: 200
    contentWidth: rect.width; contentHeight: rect.height

    // faster rebound to speed up test runs
    rebound: Transition {
        NumberAnimation {
            properties: "x,y"
            duration: 30
            easing.type: Easing.OutBounce
        }
    }

    property real minContentY: 0
    property real maxContentY: 0
    onContentYChanged: {
        minContentY = Math.min(contentY, minContentY)
        maxContentY = Math.max(contentY, maxContentY)
    }

    property real minContentX: 0
    property real maxContentX: 0
    onContentXChanged: {
        minContentX = Math.min(contentX, minContentX)
        maxContentX = Math.max(contentX, maxContentX)
    }

    property real minVerticalOvershoot: 0
    property real maxVerticalOvershoot: 0
    onVerticalOvershootChanged: {
        minVerticalOvershoot = Math.min(verticalOvershoot, minVerticalOvershoot)
        maxVerticalOvershoot = Math.max(verticalOvershoot, maxVerticalOvershoot)
    }

    property real minHorizontalOvershoot: 0
    property real maxHorizontalOvershoot: 0
    onHorizontalOvershootChanged: {
        minHorizontalOvershoot = Math.min(horizontalOvershoot, minHorizontalOvershoot)
        maxHorizontalOvershoot = Math.max(horizontalOvershoot, maxHorizontalOvershoot)
    }

    function reset() {
        minContentY = contentY
        maxContentY = contentY
        minContentX = contentX
        maxContentX = contentX
        minVerticalOvershoot = 0
        maxVerticalOvershoot = 0
        minHorizontalOvershoot = 0
        maxHorizontalOvershoot = 0
    }

    Rectangle {
        id: rect
        color: "red"
        width: 400; height: 400
    }
}
