// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12

Row {
    id: root
    property bool checked : false
    property string label : "CheckBox"
    Rectangle {
        width: 10; height: 10
        color: root.checked ? "#202020" : "transparent"
        border.color: "black"
        TapHandler {
            onTapped: root.checked = !root.checked
        }
    }
    Text { text: root.label }
}
