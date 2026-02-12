// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.ProgressBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    leftPadding: styleReader.leftPadding
    topPadding: styleReader.topPadding
    rightPadding: styleReader.rightPadding
    bottomPadding: styleReader.bottomPadding
    spacing: styleReader.spacing

    StyleVariation.controlType: styleReader.type
    StyleKitReader {
        id: styleReader
        type: StyleKitReader.ProgressBar
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.hovered
        palette: control.palette
    }

    contentItem: IndicatorDelegate {
        quickControl: control
        indicatorProperties: styleReader.indicator
        secondProgress: control.visualPosition

        Connections {
            target: control
            function onIndeterminateChanged() {
                if (indeterminate) {
                    control.contentItem.firstProgress = 0
                    control.contentItem.secondProgress = 0
                    control.contentItem.indeterminateAnim.restart()
                } else {
                    control.contentItem.indeterminateAnim.stop()
                    control.contentItem.firstProgress = 0
                    control.contentItem.secondProgress = Qt.binding(()=>{ return control.visualPosition })
                }
            }
        }

        Behavior on firstProgress {
            enabled: !control.indeterminate && !control.contentItem.indeterminateAnim.running
            NumberAnimation {
                duration: 100
                easing.type: Easing.Linear
            }
        }

        Behavior on secondProgress {
            enabled: !control.indeterminate && !control.contentItem.indeterminateAnim.running
            NumberAnimation {
                duration: 100
                easing.type: Easing.Linear
            }
        }

        property Animation indeterminateAnim: SequentialAnimation {
            running: control.indeterminate
            loops: Animation.Infinite
            NumberAnimation {
                target: control.contentItem
                property: "secondProgress"
                to: 1
                duration: 300
                easing.type: Easing.Linear
            }
            NumberAnimation {
                target: control.contentItem
                property: "firstProgress"
                to: 0.95
                duration: 600
                easing.type: Easing.OutExpo
            }
            NumberAnimation {
                target: control.contentItem
                property: "firstProgress"
                to: 0
                duration: 300
                easing.type: Easing.Linear
            }
            NumberAnimation {
                target: control.contentItem
                property: "secondProgress"
                to: 0.05
                duration: 600
                easing.type: Easing.OutExpo
            }
        }
    }

    background: BackgroundDelegate {
        quickControl: control
        backgroundProperties: styleReader.background
    }
}
