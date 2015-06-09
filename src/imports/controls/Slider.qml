/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.6
import QtQuick.Controls 2.0

AbstractSlider {
    id: control

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                            Math.max(track ? track.implicitWidth : 0,
                                     handle ? handle.implicitWidth : 0) + leftPadding + rightPadding)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                             Math.max(track ? track.implicitHeight : 0,
                                      handle ? handle.implicitHeight : 0) + topPadding + bottomPadding)

    Accessible.pressed: pressed
    Accessible.role: Accessible.Slider

    padding: Theme.padding

    handle: Rectangle {
        implicitWidth: 20
        implicitHeight: 20
        radius: width / 2
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus ? control.Theme.focusColor : control.Theme.frameColor
        color: control.Theme.backgroundColor

        readonly property bool horizontal: control.orientation === Qt.Horizontal
        x: horizontal ? control.visualPosition * (control.width - width) : (control.width - width) / 2
        y: horizontal ? (control.height - height) / 2 : control.visualPosition * (control.height - height)

        Rectangle {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: 12
            height: 12
            radius: width / 2

            color: Qt.tint(!control.enabled ? control.Theme.disabledColor :
                            control.activeFocus ? control.Theme.focusColor : control.Theme.accentColor,
                            control.pressed ? control.Theme.pressColor : "transparent")
        }
    }

    track: Rectangle {
        readonly property bool horizontal: control.orientation === Qt.Horizontal
        implicitWidth: horizontal ? 120 : 6
        implicitHeight: horizontal ? 6 : 120
        x: horizontal ? control.leftPadding : (control.width - width) / 2
        y: horizontal ? (control.height - height) / 2 : control.topPadding
        width: horizontal ? control.availableWidth : implicitWidth
        height: horizontal ? implicitHeight : control.availableHeight

        radius: control.Theme.roundness
        border.color: control.Theme.frameColor
        color: control.Theme.backgroundColor
        scale: horizontal && control.mirrored ? -1 : 1

        Rectangle {
            x: 2
            y: parent.horizontal ? 2 : control.visualPosition * parent.height + 2
            width: parent.horizontal ? control.position * parent.width - 4 : 2
            height: parent.horizontal ? 2 : control.position * parent.height - 4

            color: control.enabled ? control.Theme.accentColor : control.Theme.disabledColor
            radius: control.Theme.roundness
        }
    }
}
