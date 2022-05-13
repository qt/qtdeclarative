// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12

Item {
    id: root
    property int value: 50
    property int maximumValue: 99
    property alias label: label.text

    Rectangle {
        id: slot
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 10
        anchors.topMargin: 30
        anchors.bottomMargin: 30
        anchors.horizontalCenter: parent.horizontalCenter
        width: 10
        color: "black"
        radius: width / 2
        smooth: true
    }

    Rectangle {
        // RectangularGlow is better, but that's a different module
        id: glow
        anchors.fill: knob
        anchors.margins: -5
        anchors.leftMargin: -2
        anchors.horizontalCenterOffset: 1
        radius: 5
        color: "#4400FFFF"
        opacity: ma.pressed ? 1 : 0
    }
    Image {
        id: knob
        source: "../resources/mixer-knob.png"
        antialiasing: true
        x: slot.x - width / 2 + slot.width / 2
        height: root.width / 2
        width: implicitWidth / implicitHeight * height
        property bool programmatic: false
        property real multiplier: root.maximumValue / (ma.drag.maximumY - ma.drag.minimumY)
        onYChanged: if (!programmatic) root.value = root.maximumValue - (knob.y - ma.drag.minimumY) * multiplier
        transformOrigin: Item.Center
        function setValue(value) { knob.y = ma.drag.maximumY - value / knob.multiplier }
        MouseArea {
            id: ma
            anchors.fill: parent
            drag.target: parent
            drag.minimumX: knob.x
            drag.maximumX: knob.x
            drag.minimumY: slot.y
            drag.maximumY: slot.height + slot.y - knob.height
        }
    }

    Text {
        font.pointSize: 16
        color: "red"
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.value
    }

    Text {
        id: label
        font.pointSize: 12
        color: "red"
        anchors.top: parent.top
        anchors.topMargin: 5
        anchors.horizontalCenter: parent.horizontalCenter
    }

    onHeightChanged: {
        knob.programmatic = true
        knob.setValue(root.value)
        knob.programmatic = false
    }
}
