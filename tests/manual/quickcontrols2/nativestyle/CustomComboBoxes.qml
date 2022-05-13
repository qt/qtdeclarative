// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
