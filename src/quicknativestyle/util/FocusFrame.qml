// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    // Stack on top of all siblings of the targetItem
    z: 100

    function moveToItem(item, margins, radius) {
        if (!item) {
            targetItem = null;
            parent = null;
            visible = false;
            return;
        }
        visible = true
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

    Rectangle {
        id: focusFrame
        z: 10
        x: targetItem ? targetItem.x + leftOffset - frameSize - root.x : 0
        y: targetItem ? targetItem.y + topOffset - frameSize - root.y : 0
        width: targetItem ? targetItem.width - leftOffset - rightOffset + (frameSize * 2) : 0
        height: targetItem ? targetItem.height - topOffset - bottomOffset + (frameSize * 2) : 0
        radius: frameRadius
        visible: targetItem && targetItem.visible
        color: "transparent"

        border.color: systemFrameColor
        border.width: frameSize
    }

    ParallelAnimation {
        id: animation
        NumberAnimation {
            target: root
            property: "frameSize"
            duration: 300
            from: 15
            to: 2.5
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: focusFrame
            property: "opacity"
            duration: 300
            from: 0
            to: 0.55
            easing.type: Easing.OutCubic
        }
    }
}
