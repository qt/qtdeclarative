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
        container: root
        contentMargins {
            left: quickControl.leftPadding
            right: quickControl.rightPadding
            top: quickControl.topPadding
            bottom: quickControl.bottomPadding
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

    states: State {
        /* The delegate logic is moved out of the delegate, to relieve the style
         * developer from having to re-invent it if he changes the delegate. To
         * disable it, 'states' can be set to an empty array from the outside. */
        when: true
        PropertyChanges {
            background.parent: root
            background.x: quickControl.leftInset
            background.y: quickControl.topInset
            background.width: root.width - quickControl.leftInset - quickControl.rightInset
            background.height: root.height - quickControl.topInset - quickControl.bottomInset
        }
    }

    BackgroundDelegate {
        id: background
        quickControl: root.quickControl
        delegateStyle: root.backgroundStyle
    }

    IndicatorDelegate {
        id: indicator
        quickControl: root.quickControl
        indicatorStyle: root.indicatorStyle
        vertical: root.vertical
        z: 1
        x: !vertical ? indicatorItem.x : quickControl.leftPadding + (quickControl.availableWidth - height) / 2
        y: !vertical ? indicatorItem.y : quickControl.topPadding + width
        width: !vertical ? indicatorItem.width : __stretchBgWidth ? quickControl.availableHeight : implicitWidth//indicatorItem.height
        height: !vertical ? indicatorItem.height : __stretchBgHeight ? quickControl.availableWidth : implicitHeight//indicatorItem.width
        readonly property bool __stretchBgWidth: root.indicatorStyle.implicitWidth === Style.Stretch
        readonly property bool __stretchBgHeight: root.indicatorStyle.implicitHeight === Style.Stretch
    }
}
