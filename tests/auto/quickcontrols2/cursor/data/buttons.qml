// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

MouseArea {
    width: column.width
    height: column.height
    cursorShape: Qt.ForbiddenCursor

    Column {
        id: column
        padding: 10
        spacing: 10

        Button {
            text: "Button"
        }
        CheckBox {
            text: "CheckBox"
        }
        CheckDelegate {
            text: "CheckDelegate"
        }
        ItemDelegate {
            text: "ItemDelegate"
        }
        MenuItem {
            text: "MenuItem"
        }
        RadioButton {
            text: "RadioButton"
        }
        RadioDelegate {
            text: "RadioDelegate"
        }
        RoundButton {
            text: "X"
        }
        SwipeDelegate {
            text: "SwipeDelegate"
        }
        Switch {
            text: "Switch"
        }
        SwitchDelegate {
            text: "SwitchDelegate"
        }
        TabButton {
            text: "TabButton"
        }
        ToolButton {
            text: "ToolButton"
        }
    }
}
