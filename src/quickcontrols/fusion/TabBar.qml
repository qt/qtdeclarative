// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.Fusion
import QtQuick.Controls.Fusion.impl

T.TabBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    spacing: -1

    contentItem: ListView {
        model: control.contentModel
        currentIndex: control.currentIndex

        spacing: control.spacing
        orientation: control.orientation
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.AutoFlickIfNeeded
        snapMode: ListView.SnapToItem

        highlightMoveDuration: 0
        highlightRangeMode: ListView.ApplyRange
        preferredHighlightBegin: 40
        preferredHighlightEnd: (control.horizontal ? width : height) - 40
    }

    background: Item {
        implicitWidth: control.horizontal ? 0 : 21
        implicitHeight: control.horizontal ? 21 : 0

        Rectangle {
            width: control.horizontal ? parent.width : 1
            height: control.horizontal ? 1 : parent.height
            y: control.horizontal
                ? (control.position === T.TabBar.Header ? parent.height - 1 : 0) : 0
            x: control.vertical
                ? (control.position === T.TabBar.Header ? parent.width - 1 : 0) : 0
            color: Fusion.outline(control.palette)
        }
    }
}
