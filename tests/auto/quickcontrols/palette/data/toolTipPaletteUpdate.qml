// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window

    palette { toolTipBase: "white"; toolTipText: "black"}

    Button {
        objectName: "button"
        text: qsTr("Button with Tooltip")

        ToolTip.visible: false
        ToolTip.text: qsTr("This is a tool tip.")
    }
}
