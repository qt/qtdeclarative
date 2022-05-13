// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
