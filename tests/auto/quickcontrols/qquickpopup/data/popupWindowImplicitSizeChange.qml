// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 640
    height: 480

    property alias button: btn
    property string toolTipText: "?"

    Button {
        id: btn
        text: "Hover me"
        ToolTip.delay: 0
        ToolTip.visible: hovered && toolTipText.length > 0
        ToolTip.text: toolTipText
    }
}
