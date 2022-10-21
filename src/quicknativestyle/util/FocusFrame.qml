/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
        x: targetItem ? targetItem.x + leftOffset - frameSize : 0
        y: targetItem ? targetItem.y + topOffset - frameSize : 0
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
