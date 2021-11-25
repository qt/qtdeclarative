/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
import QtQuick.Controls.impl
import QtQuick.Controls.Fusion
import QtQuick.Controls.Fusion.impl

Rectangle {
    id: indicator

    property Item control
    readonly property color pressedColor: Fusion.mergedColors(control.palette.base, control.palette.windowText, 85)
    readonly property color checkMarkColor: Qt.darker(control.palette.text, 1.2)

    implicitWidth: 14
    implicitHeight: 14

    color: control.down ? indicator.pressedColor : control.palette.base
    border.color: control.visualFocus ? Fusion.highlightedOutline(control.palette)
                                      : Qt.lighter(Fusion.outline(control.palette), 1.1)

    Rectangle {
        x: 1; y: 1
        width: parent.width - 2
        height: 1
        color: Fusion.topShadow
        visible: indicator.control.enabled && !indicator.control.down
    }

    ColorImage {
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        color: Color.transparent(indicator.checkMarkColor, 210 / 255)
        source: "qrc:/qt-project.org/imports/QtQuick/Controls/Fusion/images/checkmark.png"
        visible: indicator.control.checkState === Qt.Checked || (indicator.control.checked && indicator.control.checkState === undefined)
    }

    Rectangle {
        x: 3; y: 3
        width: parent.width - 6
        height: parent.width - 6

        visible: indicator.control.checkState === Qt.PartiallyChecked

        gradient: Gradient {
            GradientStop {
                position: 0
                color: Color.transparent(indicator.checkMarkColor, 80 / 255)
            }
            GradientStop {
                position: 1
                color: Color.transparent(indicator.checkMarkColor, 140 / 255)
            }
        }
        border.color: Color.transparent(indicator.checkMarkColor, 180 / 255)
    }
}
