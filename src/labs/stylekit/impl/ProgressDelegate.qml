// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import Qt.labs.StyleKit

StyledItem {
    id: root

    width: parent.width
    height: parent.height

    /* Slider, RangeSlider and ProgressBar have a track (indicator.foreground) that
     * should fill up according to its progress. This StyledItem resizes itself to
     * match the geometry of the track as a way to implement this effect.
     * Using this delegate is a part of impl for now, as StyleKit don't want to dictate
     * how custom delegates in any of the mentioned controls should render their track.
     * Resizing the item is just one strategy, using e.g a shader effect would be another. */
    readonly property IndicatorDelegate __indicatorDelegate: parent.parent
    readonly property QtObject __control: parent.quickControl // DelegateContainer have a control property
    readonly property real __firstProgress: __indicatorDelegate.firstProgress ?? 0
    readonly property real __secondProgress: __indicatorDelegate.secondProgress ?? 1
    readonly property bool __vertical: __control && !!__control.vertical

    states: [
        State {
            when: !root.__vertical && (root.__firstProgress !== 0.0 || root.__secondProgress !== 1.0)
            PropertyChanges {
                root.x: root.__firstProgress * (delegateStyle.fillWidth ? parent.width - delegateStyle.minimumWidth : parent.width)
                root.width: delegateStyle.fillWidth
                ? (delegateStyle.minimumWidth + ((root.__secondProgress - root.__firstProgress) * (parent.width - delegateStyle.minimumWidth)))
                : (root.__secondProgress - root.__firstProgress) * parent.width
            }
        },
        State {
            when: root.__vertical && (root.__firstProgress !== 0.0 || root.__secondProgress !== 1.0)
            PropertyChanges {
                root.y: (1 - root.__secondProgress) * (delegateStyle.fillHeight ? parent.height - delegateStyle.minimumHeight : parent.height)
                root.height: delegateStyle.fillHeight
                    ? (delegateStyle.minimumHeight + ((root.__secondProgress - root.__firstProgress) * (parent.height - delegateStyle.minimumHeight)))
                    : (root.__secondProgress - root.__firstProgress) * parent.height
            }
        }
    ]
}
