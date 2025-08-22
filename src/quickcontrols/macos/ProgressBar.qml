// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import QtQuick.NativeStyle as NativeStyle

T.ProgressBar {
    id: root

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    readonly property bool __notCustomizable: true

    background: Item {
        implicitWidth: 100
        implicitHeight: childrenRect.height
        readonly property bool __ignoreNotCustomizable: true

        Loader {
            active: NativeStyle.StyleConstants.runningWithLiquidGlass
            width: parent.width
            height: 8
            sourceComponent: Rectangle {
                y: (parent.height - height) / 2
                radius: height / 2
                color: NativeStyle.StyleConstants.tertiarySystemFillColor
                border.color: NativeStyle.StyleConstants.secondarySystemFillColor
                border.width: 1
            }
        }

        Loader {
            active: !NativeStyle.StyleConstants.runningWithLiquidGlass
            sourceComponent: NativeStyle.ProgressBar {
                control: root
                useNinePatchImage: false
            }
        }
    }

    contentItem: Loader {
        active: NativeStyle.StyleConstants.runningWithLiquidGlass
        readonly property bool __ignoreNotCustomizable: true
        property real animPos: 0

        sourceComponent: Item {
            // The outer item is resized by the control.
            // The inner rectangle is resized according to the progress.
            Rectangle {
                property real minBlockSize: 10
                property real maxBlockSize: parent.width * 0.5;
                property real margin: (maxBlockSize - minBlockSize) / (2 * parent.width);
                property real pos: -margin + (animPos * (1 + (margin * 2)));
                property real pixelPos: pos * parent.width
                property real trackLeft: pixelPos > (maxBlockSize / 2) ? pixelPos - (maxBlockSize / 2) : 0;
                property real trackRight: pixelPos > parent.width - (maxBlockSize / 2)
                                          ? parent.width : pixelPos + (maxBlockSize / 2);

                x: root.indeterminate ? trackLeft : 0
                y: (parent.height - height) / 2
                width: root.indeterminate ? trackRight - trackLeft : parent.width * root.position
                height: 8
                radius: height / 2
                color: Application.state === Qt.ApplicationActive ? palette.accent : "lightgray"

                SequentialAnimation {
                    running: root.indeterminate
                    loops: Animation.Infinite
                    NumberAnimation {
                        target: root.contentItem
                        property: "animPos"
                        to: 1
                        duration: 800
                        easing.type: Easing.OutCubic
                    }
                    PauseAnimation { duration: 100 }
                    NumberAnimation {
                        target: root.contentItem
                        property: "animPos"
                        to: 0
                        duration: 800
                        easing.type: Easing.OutCubic
                    }
                    PauseAnimation { duration: 100 }
                }
            }
        }
    }

}
