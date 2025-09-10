// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts


Rectangle {
    id: root

    function moveToItem(item, margins, radius) {
        if (!item) {
            targetItem = null;
            parent = null;
            return;
        }
        parent = item.parent
        targetItem = item
        leftOffset = margins.left
        rightOffset = margins.right
        topOffset = margins.top
        bottomOffset = margins.bottom
        frameRadius = radius
        animation.restart()
    }

    property Item targetItem
    property real leftOffset: 0
    property real rightOffset: 0
    property real topOffset: 0
    property real bottomOffset: 0
    property real frameOpacity: 0
    property real frameSize: 0
    property real frameRadius: 0

    // systemFrameColor is set to NSColor.keyboardFocusIndicatorColor from cpp
    property color systemFrameColor

    x: targetItem ? targetItem.x + leftOffset - frameSize : 0
    y: targetItem ? targetItem.y + topOffset - frameSize : 0
    // Stack on top of all siblings of the targetItem
    z: 100

    width: targetItem ? targetItem.width - leftOffset - rightOffset + (frameSize * 2) : 0
    height: targetItem ? targetItem.height - topOffset - bottomOffset + (frameSize * 2) : 0
    radius: frameRadius + frameSize
    visible: targetItem && targetItem.visible
    color: "transparent"

    border.color: systemFrameColor
    border.width: frameSize

    ParallelAnimation {
        id: animation
        NumberAnimation {
            target: root
            property: "frameSize"
            duration: 300
            from: 15
            to: 3
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: root
            property: "opacity"
            duration: 300
            from: 0
            to: 0.55
            easing.type: Easing.OutCubic
        }
    }
}

