// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ToolBar {

    RowLayout {

        spacing: 10
        Accessible.role: Accessible.ToolBar

        ToolButton {
            text: qsTr("C&opy")
            Accessible.role: Accessible.Button
            Accessible.name: text
            Accessible.description: text
        }
        ToolButton {
            text: qsTr("C&ut")
            Accessible.role: Accessible.Button
            Accessible.name: text
            Accessible.description: text
        }
        ToolButton {
            text: qsTr("&Paste")
            Accessible.role: Accessible.Button
            Accessible.name: text
            Accessible.description: text
        }
    }
}
