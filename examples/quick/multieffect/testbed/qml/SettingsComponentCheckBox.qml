// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Column {
    id: rootItem

    property alias text: textItem.text
    property alias checked: checkBox.checked

    signal toggled

    Material.theme: Material.Dark
    Material.accent: Material.LightGreen
    spacing: -12

    Row {
        CheckBox {
            id: checkBox
            checked: true
            onToggled: {
                rootItem.toggled();
            }
        }
        Text {
            id: textItem
            anchors.verticalCenter: parent.verticalCenter
            color: "#f0f0f0"
            font.pixelSize: 14 * dp
        }
    }
}
