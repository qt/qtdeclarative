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

    leftInset: styleReader.background.leftMargin
    topInset: styleReader.background.topMargin
    rightInset: styleReader.background.rightMargin
    bottomInset: styleReader.background.bottomMargin

    spacing: styleReader.spacing

    StyleVariation.controlType: styleReader.controlType
    StyleReader {
        id: styleReader
        controlType: StyleReader.ProgressBar
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.hovered
        palette: control.palette
    }

    StyleKitLayout {
        id: progressBarLayout
        container: control.contentItem
        layoutItems: [
            StyleKitLayoutItem {
                id: indicatorItem
                item: indicator
                margins {
                    left: styleReader.indicator.leftMargin
                    right: styleReader.indicator.rightMargin
                    top: styleReader.indicator.topMargin
                    bottom: styleReader.indicator.bottomMargin
                }
                alignment: styleReader.indicator.alignment
                fillWidth: styleReader.indicator.implicitWidth === Style.Stretch
                fillHeight: styleReader.indicator.implicitHeight === Style.Stretch
            }
        ]
    }

    contentItem: Item {
        implicitWidth: Math.max(indicator.implicitWidth, progressBarLayout.implicitWidth)
        implicitHeight: Math.max(indicator.implicitHeight, progressBarLayout.implicitHeight)

        IndicatorDelegate {
            id: indicator
            quickControl: control
            indicatorStyle: styleReader.indicator
            secondProgress: control.visualPosition
            x: indicatorItem.x
            y: indicatorItem.y
            width: indicatorItem.width
            height: indicatorItem.height

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
                enabled: !control.indeterminate && !indicator.indeterminateAnim.running
                NumberAnimation {
                    duration: 100
                    easing.type: Easing.Linear
                }
            }

            Behavior on secondProgress {
                enabled: !control.indeterminate && !indicator.indeterminateAnim.running
                NumberAnimation {
                    duration: 100
                    easing.type: Easing.Linear
                }
            }

            property Animation indeterminateAnim: SequentialAnimation {
                running: control.indeterminate
                loops: Animation.Infinite
                NumberAnimation {
                    target: indicator
                    property: "secondProgress"
                    to: 1
                    duration: 300
                    easing.type: Easing.Linear
                }
                NumberAnimation {
                    target: indicator
                    property: "firstProgress"
                    to: 0.95
                    duration: 600
                    easing.type: Easing.OutExpo
                }
                NumberAnimation {
                    target: indicator
                    property: "firstProgress"
                    to: 0
                    duration: 300
                    easing.type: Easing.Linear
                }
                NumberAnimation {
                    target: indicator
                    property: "secondProgress"
                    to: 0.05
                    duration: 600
                    easing.type: Easing.OutExpo
                }
            }
        }
    }

    background: BackgroundDelegate {
        quickControl: control
        backgroundStyle: styleReader.background
    }
}
