// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

DelegateContainer {
    id: root

    implicitWidth: Math.max(delegateInstance ? delegateInstance.implicitWidth : 0, indicatorLayout.implicitWidth)
    implicitHeight: Math.max(delegateInstance ? delegateInstance.implicitHeight : 0, indicatorLayout.implicitHeight)

    transformOrigin: Item.TopLeft
    rotation: vertical ? 90 : 0
    scale: vertical ? -1 : 1
    delegateStyle: root.indicatorStyle

    required property DelegateStyle indicatorStyle
    property bool vertical: false

    /* Some indicators (Slider, RangeSlider) should let the foreground delegate
     * only fill up a certain amount of the available foreground space (that is, the
     * track / progress). This amount be controlled with firstProgress and secondProgress. */
    property real firstProgress: 0.0
    property real secondProgress: 1.0

    StyleKitLayout {
        id: indicatorLayout
        container: root
        enabled: true
        layoutItems: [
            StyleKitLayoutItem {
                id: fgItem
                item: foreground
                alignment: indicatorStyle.foreground.alignment
                margins.left: indicatorStyle.foreground.leftMargin
                margins.right: indicatorStyle.foreground.rightMargin
                margins.top: indicatorStyle.foreground.topMargin
                margins.bottom: indicatorStyle.foreground.bottomMargin
                fillWidth: indicatorStyle.foreground.implicitWidth === Style.Stretch
                fillHeight: indicatorStyle.foreground.implicitHeight === Style.Stretch
            }
        ]
        mirrored: quickControl.mirrored
    }

    DelegateContainer {
        id: foreground
        parent: root
        quickControl: root.quickControl
        delegateStyle: root.indicatorStyle.foreground
        x: fgItem.x
        y: fgItem.y
        z: 1
        width: fgItem.width
        height: fgItem.height

        states: State {
            /* Set a width on the foreground that matches the progress. But only do so if the default
             * delegate is being used. If a custom delegate is used, it is responsible for sizing
             * itself based on the available space (which is given by the size of this container).
             * (And ideally, resizing the container to match the progress should eventually be moved
             * out of this file, and into StyledItem, or perhaps a new StyledIndicatorItem).
             * Resizing the container to match the progress when a custom delegate is being used
             * assumes too much about how the delegate implements the progress, and prevents custom
             * delegates from implementing it by other means (e.g. a circular progress
             * indicator that fills in a circle rather than stretching a rectangle etc). */
            when: foreground.usingDefaultDelegate && (root.firstProgress !== 0.0 || root.secondProgress !== 1.0)
            PropertyChanges {
                target: foreground
                x: fgItem.x + root.firstProgress * (fgItem.fillWidth
                    ? fgItem.width - delegateStyle.minimumWidth : fgItem.width)
                y: fgItem.y
                width: fgItem.fillWidth ? (delegateStyle.minimumWidth
                    + ((root.secondProgress - root.firstProgress) * (fgItem.width
                    - delegateStyle.minimumWidth)))
                        : (root.secondProgress - root.firstProgress) * fgItem.width
                height: fgItem.height
            }
        }
    }
}
