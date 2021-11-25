/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "TextFields"

    Row {
        spacing: container.rowSpacing

        TextArea {
            id: customBackground
            width: 200
            wrapMode: TextEdit.WordWrap
            text: "Custom background - Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
                  + "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
            background: Rectangle {
                implicitWidth: customBackground.contentWidth
                implicitHeight: customBackground.contentHeight
                border.width: customBackground.activeFocus ? 2 : 1
                color: control.palette.base
                border.color: "green"
            }
        }

        TextArea {
            width: 200
            placeholderText: "Large font"
            font.pixelSize: 20
            wrapMode: TextEdit.WordWrap
            text: "Large font - Lorem ipsum dolor sit amet, consectetur adipiscing elit"
        }
    }
}
