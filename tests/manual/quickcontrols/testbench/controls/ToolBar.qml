// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// TODO
QtObject {
    property string customControlName: qsTr("ToolBar, ToolButton & ToolSeparator")

    property var supportedStates: [
        ["header"],
        ["header", "disabled"],
        ["footer"],
        ["footer", "disabled"]
    ]

    property Component component: ToolBar {
        enabled: !is("disabled")
        position: is("header") ? ToolBar.Header : ToolBar.Footer

        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: qsTr("ToolButton 1")
            }
            ToolButton {
                text: qsTr("ToolButton 2")
            }

            ToolSeparator {}

            ToolButton {
                text: qsTr("ToolButton 3")
                checkable: true
                checked: true
            }
        }
    }
}
