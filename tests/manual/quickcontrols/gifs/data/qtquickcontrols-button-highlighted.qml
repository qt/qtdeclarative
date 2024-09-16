// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    width: button.width
    height: button.height
    visible: true

    Button {
        id: button
        text: pressed ? "Pressed" : "Button"
        highlighted: true
        anchors.centerIn: parent
    }
}
