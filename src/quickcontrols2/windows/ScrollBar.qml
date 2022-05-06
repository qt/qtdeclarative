/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

import QtQuick
import QtQuick.NativeStyle as NativeStyle

NativeStyle.DefaultScrollBar {
    id: controlRoot

    topPadding:    orientation === Qt.Vertical   ? controlRoot.__decreaseVisual.indicator.height : 0
    bottomPadding: orientation === Qt.Vertical   ? controlRoot.__increaseVisual.indicator.height : 0
    leftPadding:   orientation === Qt.Horizontal ? controlRoot.__decreaseVisual.indicator.width : 0
    rightPadding:  orientation === Qt.Horizontal ? controlRoot.__increaseVisual.indicator.width : 0

    contentItem: NativeStyle.ScrollBar {
        control: controlRoot
        subControl: NativeStyle.ScrollBar.Handle
    }

    NativeStyle.ScrollBar {
        // Fade a hovered-looking version of the handle
        // on top of the default handle when hovering it
        x: contentItem.x
        y: contentItem.y
        z: 1
        width: contentItem.width
        height: contentItem.height
        control: controlRoot
        subControl: NativeStyle.ScrollBar.Handle
        overrideState: NativeStyle.StyleItem.AlwaysHovered
        opacity: controlRoot.hovered || control.pressed ? 1 : 0
        visible: contentItem instanceof NativeStyle.StyleItem
        Behavior on opacity { NumberAnimation { duration: contentItem.transitionDuration } }
    }

    // The groove background should have window color
    Rectangle {
        x: background.x
        y: background.y
        z: -1
        width: background.width
        height: background.height
        color: controlRoot.palette.window
    }

    background: NativeStyle.ScrollBar {
        control: controlRoot
        subControl: NativeStyle.ScrollBar.Groove
        overrideState: NativeStyle.ScrollBar.NeverHovered
    }

    __decreaseVisual.indicator: NativeStyle.ScrollBar {
        control: controlRoot
        subControl: NativeStyle.ScrollBar.SubLine
        overrideState: NativeStyle.ScrollBar.AlwaysHovered
        opacity: controlRoot.__decreaseVisual.hovered ? 1 : 0
        visible: contentItem instanceof NativeStyle.StyleItem
        Behavior on opacity { NumberAnimation { duration: contentItem.transitionDuration } }
        useNinePatchImage: false
    }

    NativeStyle.ScrollBar {
        control: controlRoot
        subControl: NativeStyle.ScrollBar.SubLine
        overrideState: NativeStyle.ScrollBar.AlwaysSunken
        opacity: controlRoot.__decreaseVisual.pressed ? 1 : 0
        visible: contentItem instanceof NativeStyle.StyleItem
        useNinePatchImage: false
        z: 1
    }

    __increaseVisual.indicator: NativeStyle.ScrollBar {
        control: controlRoot
        subControl: NativeStyle.ScrollBar.AddLine
        x: orientation === Qt.Horizontal ? controlRoot.width - width : 0
        y: orientation === Qt.Vertical ? controlRoot.height - height : 0
        overrideState: NativeStyle.ScrollBar.AlwaysHovered
        opacity: controlRoot.__increaseVisual.hovered ? 1 : 0
        visible: contentItem instanceof NativeStyle.StyleItem
        Behavior on opacity { NumberAnimation { duration: contentItem.transitionDuration } }
        useNinePatchImage: false
    }

    NativeStyle.ScrollBar {
        control: controlRoot
        subControl: NativeStyle.ScrollBar.AddLine
        x: __increaseVisual.indicator.x
        y: __increaseVisual.indicator.y
        z: 1
        overrideState: NativeStyle.ScrollBar.AlwaysSunken
        opacity: controlRoot.__increaseVisual.pressed ? 1 : 0
        visible: contentItem instanceof NativeStyle.StyleItem
        useNinePatchImage: false
    }
}
