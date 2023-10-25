// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick

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
    }

    property Item targetItem
    property real leftOffset: 0
    property real rightOffset: 0
    property real topOffset: 0
    property real bottomOffset: 0
    property real frameOpacity: 0
    property real frameSize: 0
    property real frameRadius: 0

    Canvas {
        x: targetItem ? targetItem.x + leftOffset - frameSize - root.x : 0
        y: targetItem ? targetItem.y + topOffset - frameSize - root.y : 0
        width: targetItem ? targetItem.width - leftOffset - rightOffset + (frameSize * 2) : 0
        height: targetItem ? targetItem.height - topOffset - bottomOffset + (frameSize * 2) : 0
        visible: targetItem && targetItem.visible

        onPaint: {
            let context = getContext("2d")
            context.strokeStyle = Qt.rgba(0, 0, 0, 1)
            context.setLineDash([1, 1])
            context.beginPath()
            context.roundedRect(0.5, 0.5, width - 1, height - 1, root.frameRadius, root.frameRadius)
            context.stroke()
        }
    }
}
