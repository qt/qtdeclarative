// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "TextFields"

    Row {
        spacing: container.rowSpacing

        TextField {
            placeholderText: "Custom background"
            background: Rectangle {
                implicitWidth: 130
                implicitHeight: 20
                border.width: control.activeFocus ? 2 : 1
                color: control.palette.base
                border.color: "green"
            }
        }
        TextField {
            placeholderText: "Large font"
            font.pixelSize: 20
        }
    }
}
