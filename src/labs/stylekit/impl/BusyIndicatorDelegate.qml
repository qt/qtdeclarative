// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Templates as T
import Qt.labs.StyleKit

Item {
    id: root

    required property DelegateStyle delegateStyle
    required property T.BusyIndicator control

    readonly property int segmentCount: 8
    readonly property real size: Math.min(width, height)

    readonly property real segmentWidth: Math.max(2, size * 0.1)
    readonly property real segmentHeight: size * 0.25

    readonly property real margin: size * 0.1
    readonly property real segmentDistance: size / 2 - margin - segmentHeight / 2

    property int activeSegment: 0

    implicitWidth: delegateStyle.width
    implicitHeight: delegateStyle.height
    width: parent.width
    height: parent.height

    NumberAnimation on activeSegment {
        from: 0
        to: root.segmentCount
        duration: 1000
        loops: Animation.Infinite
        running: root.control.running
        easing.type: Easing.Linear
    }

    Repeater {
        model: root.segmentCount

        delegate: Item {
            id: segmentDelegate

            required property int index
            readonly property int opacityPosition: (root.activeSegment - index + root.segmentCount) % root.segmentCount

            width: parent.width
            height: parent.height
            rotation: index * 360 / root.segmentCount

            StyledItem {
                delegateStyle: root.delegateStyle
                width: root.segmentWidth
                height: root.segmentHeight
                x: (parent.width - width) / 2
                y: (parent.height - height) / 2 - root.segmentDistance
                opacity: root.control.running
                            ? (1 - 0.85 * segmentDelegate.opacityPosition / (root.segmentCount - 1))
                            : 0

                Behavior on opacity {
                    OpacityAnimator {
                        duration: 250
                    }
                }
            }
        }
    }
}
