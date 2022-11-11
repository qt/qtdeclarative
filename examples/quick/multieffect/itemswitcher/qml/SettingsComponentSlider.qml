// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Column {
    id: rootItem

    property alias text: textItem.text
    property alias value: slider.value
    property alias from: slider.from
    property alias to: slider.to
    property alias checked: checkBox.checked
    property alias stepSize: slider.stepSize
    property bool showCheckbox: false

    signal toggled
    signal moved

    Material.theme: Material.Dark
    Material.accent: Material.LightGreen
    spacing: -12

    Text {
        id: textItem
        anchors.horizontalCenter: parent.horizontalCenter
        color: "#f0f0f0"
        font.pixelSize: 14 * dp
    }

    Row {
        CheckBox {
            id: checkBox
            visible: rootItem.showCheckbox
            checked: true
            onToggled: {
                rootItem.toggled();
            }
        }
        Slider {
            id: slider
            property real sliderWidth: settings.settingsViewWidth - 50
            width: rootItem.showCheckbox ? sliderWidth : sliderWidth + checkBox.width
            value: 50
            from: 0
            to: 800
            onMoved: {
                rootItem.moved();
            }
        }
    }
}
