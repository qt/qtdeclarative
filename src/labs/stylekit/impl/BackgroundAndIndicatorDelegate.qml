// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

/*
    This delegate is a composition of the background and the indicator into
    a single delegate. This is needed by some controls, since they only have
    a background delegate, which is responsible for also drawing the "indicator".
    An example is a Slider, which draws both the background, groove and track in
    the background delegate.
*/
Item {
    id: root
    implicitWidth: Math.max(background.implicitWidth, indicatorLayout.implicitWidth)
    implicitHeight: Math.max(background.implicitHeight, indicatorLayout.implicitHeight)

    required property DelegateStyle indicatorStyle
    required property DelegateStyle backgroundStyle
    required property T.Control quickControl
    property alias indicator: indicator

    StyleKitLayout {
        id: indicatorLayout
        container: root
        contentMargins {
            left: quickControl.leftPadding - quickControl.leftInset
            top: quickControl.topPadding - quickControl.topInset
            right: quickControl.rightPadding - quickControl.rightInset
            bottom: quickControl.bottomPadding - quickControl.bottomInset
        }
        layoutItems: [
            StyleKitLayoutItem {
                id: indicatorItem
                item: root.indicator
                alignment: indicatorStyle.alignment
                margins.left: indicatorStyle.leftMargin
                margins.right: indicatorStyle.rightMargin
                margins.top: indicatorStyle.topMargin
                margins.bottom: indicatorStyle.bottomMargin
                fillWidth: indicatorStyle.fillWidth
                fillHeight: indicatorStyle.fillHeight
            }
        ]
        mirrored: quickControl.mirrored
    }

    BackgroundDelegate {
        id: background
        quickControl: root.quickControl
        backgroundStyle: root.backgroundStyle
        width: parent.width
        height: parent.height
    }

    IndicatorDelegate {
        id: indicator
        quickControl: root.quickControl
        indicatorStyle: root.indicatorStyle
        z: 1
        x: indicatorItem.x
        y: indicatorItem.y
        width: indicatorItem.width
        height: indicatorItem.height
    }
}
