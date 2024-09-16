// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

//! [1]
Button {
    text: qsTr("Button")

    ToolTip.visible: pressed
    ToolTip.delay: Application.styleHints.mousePressAndHoldInterval
    ToolTip.text: qsTr("This tool tip is shown after pressing and holding the button down.")
}
//! [1]
