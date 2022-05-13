// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    width: column.implicitWidth
    height: column.implicitHeight
    visible: true

    property var delegate: repeater.count > 0 ? repeater.itemAt(0) : null

    Column {
        id: column
        anchors.centerIn: parent

        Repeater {
            id: repeater
            model: ["Option 1", "Option 2", "Option 3"]
            delegate: SwitchDelegate {
                text: modelData
            }
        }
    }
}
