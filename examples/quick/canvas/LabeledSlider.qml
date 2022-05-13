// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Item {
    id: labeledSlider
    property alias name: label.text
    implicitHeight: Math.max(label.implicitHeight, quickControlsSlider.implicitHeight)
    property real min: 0.0
    property real max: 1.0
    property real init: 0.0
    readonly property alias value: quickControlsSlider.value

    Label {
        id: label
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        color: "#333"
        font: Qt.font({pointSize: 13})
    }

    Slider {
        id: quickControlsSlider
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.left: label.right
        anchors.leftMargin: 20
        from: labeledSlider.min
        to: labeledSlider.max
        width: labeledSlider.width - label.implicitWidth - (label.anchors.leftMargin + anchors.rightMargin + anchors.leftMargin)

        Component.onCompleted: ()=> value = labeledSlider.init;
    }
}
