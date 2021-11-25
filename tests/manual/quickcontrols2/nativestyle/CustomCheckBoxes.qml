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
import QtQuick.Controls.impl

ControlContainer {
    id: container
    title: "CheckBoxes"

    Row {
        spacing: container.rowSpacing

        CheckBox {
            id: customContentItem
            text: "Custom content item"
            contentItem: Text {
                text: customContentItem.text
                color: "green"
                leftPadding: customContentItem.indicator.width + customContentItem.spacing
            }
        }

        CheckBox {
            id: customIndicator
            text: "Custom indicator"
            indicator: Rectangle {
                implicitWidth: 15
                implicitHeight: 15

                x: customIndicator.text ? customIndicator.leftPadding : customIndicator.leftPadding + (customIndicator.availableWidth - width) / 2
                y: customIndicator.topPadding + (customIndicator.availableHeight - height) / 2

                color: customIndicator.down ? customIndicator.palette.light : customIndicator.palette.base
                border.color: "green"
                border.width: 2

                ColorImage {
                    x: (parent.width - width) / 2
                    y: (parent.height - height) / 2
                    defaultColor: "#353637"
                    scale: 0.5
                    color: "green"
                    source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png"
                    visible: customIndicator.checkState === Qt.Checked
                }

                Rectangle {
                    x: (parent.width - width) / 2
                    y: (parent.height - height) / 2
                    width: 16
                    height: 3
                    color: customIndicator.palette.text
                    visible: customIndicator.checkState === Qt.PartiallyChecked
                }
            }
        }

        CheckBox {
            id: allCustom
            text: "All custom"

            contentItem: Text {
                text: allCustom.text
                color: "green"
                leftPadding: allCustom.indicator.width + allCustom.spacing
                rightPadding: allCustom.indicator.width + allCustom.spacing
            }

            indicator: Rectangle {
                implicitWidth: 15
                implicitHeight: 15

                x: allCustom.text ? allCustom.leftPadding : allCustom.leftPadding + (allCustom.availableWidth - width) / 2
                y: allCustom.topPadding + (allCustom.availableHeight - height) / 2

                color: allCustom.down ? allCustom.palette.light : allCustom.palette.base
                border.color: "green"
                border.width: 2

                ColorImage {
                    x: (parent.width - width) / 2
                    y: (parent.height - height) / 2
                    defaultColor: "#353637"
                    scale: 0.5
                    color: "green"
                    source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png"
                    visible: allCustom.checkState === Qt.Checked
                }

                Rectangle {
                    x: (parent.width - width) / 2
                    y: (parent.height - height) / 2
                    width: 16
                    height: 3
                    color: "green"
                    visible: allCustom.checkState === Qt.PartiallyChecked
                }
            }
        }
    }
}
