// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
