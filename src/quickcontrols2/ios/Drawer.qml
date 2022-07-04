// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.iOS

T.Drawer {
    id: control

    parent: T.Overlay.overlay

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    property real inset: control.dim ? 8 : 0
    property bool vertical: control.edge === Qt.LeftEdge || control.edge === Qt.RightEdge

    rightInset: background && control.edge === Qt.LeftEdge ? -inset : 0
    leftInset: background && control.edge === Qt.RightEdge ? -inset : 0
    bottomInset: background && control.edge === Qt.TopEdge ? -inset : 0
    topInset: background && control.edge === Qt.BottomEdge ? -inset : 0

    enter: Transition { SmoothedAnimation { velocity: 5 } }
    exit: Transition { SmoothedAnimation { velocity: 5 } }

    background: Item {
        NinePatchImage {
            source: control.IOS.url + "drawer-background"
            NinePatchImageSelector on source {
                states: [
                    {"light": control.IOS.theme === IOS.Light},
                    {"dark": control.IOS.theme === IOS.Dark},
                    {"modal": control.modal}
                ]
            }
            y: (parent.height - height) / 2
            x: (parent.width - width) / 2
            rotation: control.edge === Qt.TopEdge ? 90 : (control.edge === Qt.BottomEdge ? -90
                                                  : (control.edge === Qt.RightEdge ? 180 : 0))
            width: vertical ? parent.width : parent.height
            height: vertical ? parent.height : parent.width
        }
        Rectangle {
            width: vertical ? 1 : parent.width
            height: vertical ? parent.height : 1
            color: control.palette.mid
            x: control.edge === Qt.LeftEdge ? parent.width - 1 - inset : (control.edge === Qt.RightEdge ? inset : 0)
            y: control.edge === Qt.BottomEdge ? inset : (control.edge === Qt.TopEdge ? parent.height - 1 - inset : 0)
            z: 10
        }
    }

    T.Overlay.modal: Rectangle {
        color: Color.transparent(control.palette.mid, 0.5)
        Behavior on opacity { NumberAnimation { duration: 150 } }
    }
}
