/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.impl

ControlContainer {
    id: container
    title: "ComboBoxes"

    Row {
        spacing: container.rowSpacing

        ComboBox {
            id: control
            model: [ "Custom background", "Banana", "Apple", "Coconut" ]
            background: Rectangle {
                implicitWidth: 200
                implicitHeight: 20
                color: control.down ? control.palette.mid : control.palette.button
                border.color: "green"
                border.width: 1
            }
            indicator: ColorImage {
                x: control.mirrored ? control.padding : control.width - width - control.padding
                y: control.topPadding + (control.availableHeight - height) / 2
                color: control.palette.dark
                defaultColor: "#353637"
                source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/double-arrow.png"
                opacity: enabled ? 1 : 0.3
            }
        }

        ComboBox {
            model: [ "Banana", "Apple", "Coconut" ]
            contentItem: Rectangle {
                implicitWidth: text.implicitWidth
                color: "lightGreen"
                Text {
                    id: text
                    text: "Custom content item"
                    anchors.centerIn: parent
                }
            }
        }

    }

    Row {
        spacing: container.rowSpacing

        ComboBox {
            id: control2
            model: [ "Custom background", "Banana", "Apple", "Coconut" ]
            editable: true
            background: Rectangle {
                implicitWidth: 200
                implicitHeight: 20
                color: control2.down ? control2.palette.mid : control2.palette.button
                border.color: "green"
                border.width: 1
            }
            indicator: ColorImage {
                x: control2.mirrored ? control2.padding : control2.width - width - control2.padding
                y: control2.topPadding + (control2.availableHeight - height) / 2
                color: control2.palette.dark
                defaultColor: "#353637"
                source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/double-arrow.png"
                opacity: enabled ? 1 : 0.3
            }
        }

        ComboBox {
            model: [ "Banana", "Apple", "Coconut" ]
            editable: true
            contentItem: Rectangle {
                implicitWidth: text2.implicitWidth
                color: "lightGreen"
                TextEdit {
                    id: text2
                    text: "Custom content item"
                    anchors.centerIn: parent
                }
            }
        }
    }
}
