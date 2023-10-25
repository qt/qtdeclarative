// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    // It's important that this item has a zero size. Otherwise, if the parent of the
    // targetItem is e.g a layout, we will change the layout if we parent this item inside it.
    width: 0
    height: 0
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
        radius: frameRadius + frameSize
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
            to: 3
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
