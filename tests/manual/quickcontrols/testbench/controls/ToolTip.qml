// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

QtObject {
    property var supportedStates: [
        []
    ]

    property Component component: Button {
        text: qsTr("Hover over me")

        ToolTip.text: qsTr("ToolTip")
        ToolTip.visible: hovered
        ToolTip.delay: 500
    }
}
