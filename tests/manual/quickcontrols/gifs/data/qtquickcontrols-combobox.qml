// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Window {
    width: 140
    height: 180
    visible: true

    property alias comboBox: comboBox

    ComboBox {
        id: comboBox
        model: ["First", "Second", "Third"]
        y: 10
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
