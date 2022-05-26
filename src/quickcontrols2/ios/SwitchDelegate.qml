/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.iOS

T.SwitchDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)
    padding: 7
    leftPadding: 16
    rightPadding: 16
    spacing: 14

    icon.width: 29
    icon.height: 29
    icon.color: control.enabled ? control.palette.text : control.palette.mid

    indicator: Image {
        x: control.text ? (control.mirrored ? control.leftPadding : control.width - width - control.rightPadding) : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        width: implicitWidth
        height: Math.max(implicitHeight, handle.implicitHeight)
        opacity: control.enabled ? 1 : 0.5

        source: control.IOS.url + "switch-indicator"
        ImageSelector on source {
            states: [
                {"light": control.IOS.theme === IOS.Light},
                {"dark": control.IOS.theme === IOS.Dark},
                {"checked": control.checked}
            ]
        }

        property NinePatchImage handle: NinePatchImage {
            property real margin: 2
            readonly property real minPos: leftPadding - leftInset + margin
            readonly property real maxPos: parent.width - width + rightPadding + rightInset - margin
            readonly property real dragPos: control.visualPosition * parent.width - (width / 2)

            parent: control.indicator

            x: Math.max(minPos, Math.min(maxPos, dragPos))
            y: (parent.height - height) / 2 - topInset + margin
            width: control.pressed ? implicitWidth + 4 : implicitWidth

            source: control.IOS.url + "switch-handle"
            NinePatchImageSelector on source {
                states: [
                    {"light": control.IOS.theme === IOS.Light},
                    {"dark": control.IOS.theme === IOS.Dark},
                    {"disabled": !control.enabled}
                ]
            }

            Behavior on x {
                enabled: !control.down
                SmoothedAnimation { velocity: 150 }
            }
        }
    }

    contentItem: IconLabel {
        leftPadding: control.mirrored ? control.indicator.width + control.spacing : 0
        rightPadding: !control.mirrored ? control.indicator.width + control.spacing : 0

        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        alignment: control.display === IconLabel.IconOnly || control.display === IconLabel.TextUnderIcon ? Qt.AlignCenter : Qt.AlignLeft

        icon: control.icon
        text: control.text
        font: control.font
        color: control.enabled ? control.palette.text : control.palette.mid
    }

    background: Rectangle {
        implicitHeight: 44
        color: control.IOS.theme === IOS.Dark ? control.palette.dark : control.palette.base
        NinePatchImage {
            property real offset: control.icon.source.toString() !== "" ? control.icon.width + control.spacing : 0
            x: control.leftPadding + offset
            height: control.height
            width: control.availableWidth + control.rightPadding - offset
            source: control.IOS.url + "itemdelegate-background"
            NinePatchImageSelector on source {
                states: [
                    {"light": control.IOS.theme === IOS.Light},
                    {"dark": control.IOS.theme === IOS.Dark},
                ]
            }
        }
    }
}
