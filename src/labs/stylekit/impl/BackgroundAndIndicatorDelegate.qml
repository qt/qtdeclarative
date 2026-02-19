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
    implicitWidth: !vertical
                   ? Math.max(background.implicitWidth,
                              indicatorLayout.implicitWidth)
                   : Math.max(background.implicitHeight,
                              indicatorLayout.implicitHeight)
    implicitHeight: !vertical
                    ? Math.max(background.implicitHeight,
                               indicatorLayout.implicitHeight)
                    : Math.max(background.implicitWidth,
                               indicatorLayout.implicitWidth)

    required property DelegateStyle indicatorStyle
    required property DelegateStyle backgroundStyle
    required property T.Control quickControl
    property alias indicator: indicator
    property bool vertical: false

    StyleKitLayout {
        id: indicatorLayout
        container: Item {
            width: !vertical ? root.width : root.height
            height: !vertical ? root.height : root.width
        }
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
                fillWidth: indicatorStyle.implicitWidth === Style.Stretch
                fillHeight: indicatorStyle.implicitHeight === Style.Stretch
            }
        ]
        mirrored: quickControl.mirrored
    }

    BackgroundDelegate {
        id: background
        quickControl: root.quickControl
        delegateStyle: root.backgroundStyle
        width: parent.width
        height: parent.height
    }

    IndicatorDelegate {
        id: indicator
        quickControl: root.quickControl
        indicatorStyle: root.indicatorStyle
        vertical: root.vertical
        z: 1
        x: !vertical ? indicatorItem.x : indicatorItem.y
        y: !vertical ? indicatorItem.y : indicatorItem.x + indicatorItem.width
        width: !vertical ? indicatorItem.width : indicatorItem.width
        height: !vertical ? indicatorItem.height : indicatorItem.height
    }
}
