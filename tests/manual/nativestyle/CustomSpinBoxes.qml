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

ControlContainer {
    id: container
    title: "SpinBoxes"

    Row {
        spacing: container.rowSpacing

        SpinBox {
            id: custombg
            value: 1000
            to: 2000
            background: Rectangle {
                border.color: "green"
                implicitWidth: 50
            }
        }

        SpinBox {
            id: customIndicator
            value: 500
            to: 2000

            rightPadding: 17
            spacing: 0
            implicitWidth: 60
            implicitHeight: 25

            up.indicator: Rectangle {
                x: customIndicator.width - width - 4
                y: 4
                implicitWidth: customIndicator.rightPadding - 4
                implicitHeight: 8
                border.width: 1
                border.color: "green"
                color: customIndicator.up.pressed ? "gray" : "transparent"
                Text {
                    text: "+"
                    font.pixelSize: 8
                    font.bold: true
                    anchors.centerIn: parent
                }
            }

            down.indicator: Rectangle {
                x: customIndicator.width - width - 4
                y: height + 6
                implicitWidth: customIndicator.rightPadding - 4
                implicitHeight: 8
                border.width: 1
                border.color: "green"
                color: customIndicator.down.pressed ? "gray" : "transparent"
                Text {
                    text: "-"
                    font.pixelSize: 10
                    font.bold: true
                    anchors.centerIn: parent
                }
            }
        }

        SpinBox {
            id: allCustom
            value: 500
            to: 2000

            rightPadding: 17
            spacing: 0
            implicitWidth: 60
            implicitHeight: 25

            background: Rectangle {
                border.color: "green"
                implicitWidth: 50
            }

            up.indicator: Rectangle {
                x: allCustom.width - width - 4
                y: 4
                implicitWidth: allCustom.rightPadding - 4
                implicitHeight: 8
                border.width: 1
                border.color: "green"
                color: allCustom.up.pressed ? "gray" : "transparent"
                Text {
                    text: "+"
                    font.pixelSize: 8
                    font.bold: true
                    anchors.centerIn: parent
                }
            }

            down.indicator: Rectangle {
                x: allCustom.width - width - 4
                y: height + 6
                implicitWidth: allCustom.rightPadding - 4
                implicitHeight: 8
                border.width: 1
                border.color: "green"
                color: allCustom.down.pressed ? "gray" : "transparent"
                Text {
                    text: "-"
                    font.pixelSize: 10
                    font.bold: true
                    anchors.centerIn: parent
                }
            }

            contentItem: TextInput {
                text: allCustom.displayText
                font: allCustom.font
                color: "green"
                selectionColor: allCustom.palette.highlight
                selectedTextColor: allCustom.palette.highlightedText
                horizontalAlignment: Qt.AlignLeft
                verticalAlignment: Qt.AlignVCenter

                topPadding: 2
                bottomPadding: 2
                leftPadding: 10
                rightPadding: 10

                readOnly: !allCustom.editable
                validator: allCustom.validator
                inputMethodHints: allCustom.inputMethodHints
            }

        }

    }

}
