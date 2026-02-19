// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.ScrollView {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    // rightPadding and bottomPadding are used to make space for the scrollBars
    // but because we're setting them explicitly here, there will be no effect
    // if the user assign a value to the padding property, so we accumulate it
    // with the scrollbar width and height
    rightPadding: effectiveScrollBarWidth + __styleReader.rightPadding
    bottomPadding: effectiveScrollBarHeight + __styleReader.bottomPadding
    topPadding: __styleReader.topPadding
    leftPadding: __styleReader.leftPadding

    rightInset: __styleReader.background.rightMargin
    leftInset: __styleReader.background.leftMargin
    topInset: __styleReader.background.topMargin
    bottomInset: __styleReader.background.bottomMargin

    StyleVariation.controlType: __styleReader.controlType
    property StyleReader __styleReader: StyleReader {
        controlType: StyleReader.ScrollView
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.hovered
        palette: control.palette
    }

    // ScrollView has no styling, but we anyway need to subclass it
    // and attach the scrollbars, in order to use the ones defined
    // in the StyleKit module.
    ScrollBar.vertical: ScrollBar {
        parent: control
        x: control.mirrored ? 0 : control.width - width
        y: control.topPadding
        height: control.availableHeight
        active: control.ScrollBar.horizontal.active
    }

    ScrollBar.horizontal: ScrollBar {
        parent: control
        x: control.leftPadding
        y: control.height - height
        width: control.availableWidth
        active: control.ScrollBar.vertical.active
    }
}
