// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Templates as T

T.TabBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    spacing: 1

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

    background: Rectangle {
        implicitWidth: control.horizontal ? 0 : 49
        implicitHeight: control.horizontal ? 49 : 0
        color: Application.styleHints.colorScheme === Qt.Dark ? control.palette.light : control.palette.base
        Rectangle {
            height: control.horizontal ? 1 : parent.height
            width: control.horizontal ? parent.width : 1
            color: control.palette.mid
            y: control.horizontal
                ? (control.position === T.TabBar.Footer ? 0 : parent.height - 1) : 0
            x: control.vertical
                ? (control.position === T.TabBar.Footer ? 0 : parent.width - 1) : 0
        }
    }
}
