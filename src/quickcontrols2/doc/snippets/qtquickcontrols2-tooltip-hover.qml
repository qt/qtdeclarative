// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

//! [1]
Button {
    text: qsTr("Button")
    hoverEnabled: true

    ToolTip.delay: 1000
    ToolTip.timeout: 5000
    ToolTip.visible: hovered
    ToolTip.text: qsTr("This tool tip is shown after hovering the button for a second.")
}
//! [1]
