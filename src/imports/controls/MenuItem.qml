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
import Qt.labs.templates 1.0 as T

T.MenuItem {
    id: control

    implicitWidth: background ? background.implicitWidth
        : (label ? label.implicitWidth : 0) + (indicator ? indicator.implicitWidth : 0)
            + (label && indicator ? spacing : 0) + leftPadding + rightPadding
    implicitHeight: background ? background.implicitHeight
        : (label ? label.implicitHeight : 0) + (indicator ? indicator.implicitHeight : 0) + topPadding + bottomPadding
    baselineOffset: label ? label.y + label.baselineOffset : 0

    padding: 6

    //! [label]
    label: Text {
        x: control.mirrored ? control.leftPadding : control.width - width - control.rightPadding
        y: control.topPadding
        width: control.availableWidth - (control.checkable ? indicator.width + control.spacing : 0)
        height: control.availableHeight

        text: control.text
        font: control.font
        color: control.enabled ? "#26282a" : "#bdbebf"
        elide: Text.ElideRight
        visible: control.text
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
    }
    //! [label]

    //! [indicator]
    indicator: Image {
        x: control.mirrored ? control.width - width - control.rightPadding : control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2

        visible: control.checked
        source: control.checkable ? "qrc:/qt-project.org/imports/Qt/labs/controls/images/check.png" : ""
    }
    //! [indicator]

    //! [background]
    background: Item {
        implicitWidth: 200
        implicitHeight: 40

        Rectangle {
            x: 1
            y: 1
            width: parent.width - 2
            height: parent.height - 2
            color: control.activeFocus || control.pressed ? "#eeeeee" : "transparent"
        }
    }
    //! [background]
}
