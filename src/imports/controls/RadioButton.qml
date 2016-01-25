/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Controls module of the Qt Toolkit.
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
import Qt.labs.controls 1.0
import Qt.labs.templates 1.0 as T

T.RadioButton {
    id: control

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                            (label ? label.implicitWidth : 0) +
                            (indicator ? indicator.implicitWidth : 0) +
                            (label && indicator ? spacing : 0) + leftPadding + rightPadding)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                             Math.max(label ? label.implicitHeight : 0,
                                      indicator ? indicator.implicitHeight : 0) + topPadding + bottomPadding)
    baselineOffset: label ? label.y + label.baselineOffset : 0

    padding: 6
    spacing: 6
    opacity: enabled ? 1 : 0.2

    //! [indicator]
    indicator: Rectangle {
        implicitWidth: 28
        implicitHeight: 28
        x: text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding) : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2

        radius: width / 2
        border.width: 1
        border.color: (control.pressed ? "#26282a" : "#353637")
        color: control.pressed ? "#e4e4e4" : "#f6f6f6"

        Rectangle {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: 20
            height: 20
            radius: width / 2
            color: control.pressed ? "#26282a" : "#353637"
            visible: control.checked
        }
    }
    //! [indicator]

    //! [label]
    label: Text {
        x: control.mirrored ? control.leftPadding : (indicator.x + indicator.width + control.spacing)
        y: control.topPadding
        width: control.availableWidth - indicator.width - control.spacing
        height: control.availableHeight

        text: control.text
        font: control.font
        color: control.pressed ? "#26282a" : "#353637"
        elide: Text.ElideRight
        visible: control.text
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
    }
    //! [label]
}
