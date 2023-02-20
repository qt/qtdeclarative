// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Item {
    id: labeledSlider
    property alias name: label.text
    property real min: 0.0
    property real max: 1.0
    property alias value: quickControlsSlider.value

    implicitHeight: Math.max(label.implicitHeight, quickControlsSlider.implicitHeight)

    Label {
        id: label
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.verticalCenter: labeledSlider.verticalCenter
        font: Qt.font({pointSize: 13})
    }

    Slider {
        id: quickControlsSlider
        anchors {
            verticalCenter: labeledSlider.verticalCenter
            right: labeledSlider.right
            rightMargin: 10
            left: label.right
            leftMargin: 10
        }
        from: labeledSlider.min
        to: labeledSlider.max
        width: labeledSlider.width - label.implicitWidth - (label.anchors.leftMargin + anchors.rightMargin + anchors.leftMargin)
    }
}
