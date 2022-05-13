// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12

Item {
    id: root
    property int value: 50
    property int maximumValue: 99
    property alias label: label.text
    property alias tapEnabled: tap.enabled
    property alias pressed: tap.pressed
    signal tapped

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
        id: glow
        anchors.fill: knob
        anchors.margins: -5
        anchors.leftMargin: -2
        anchors.horizontalCenterOffset: 1
        radius: 5
        color: "#4400FFFF"
        opacity: tap.pressed || tapFlash.running ? 1 : 0
        FlashAnimation on visible {
            id: tapFlash
        }
    }
    Rectangle {
        id: knob
        objectName: "Slider Knob"
        width: parent.width - 2
        height: 20
        radius: 5
        color: "darkgray"
        border.color: "black"
        property bool programmatic: false
        property real multiplier: root.maximumValue / (dragHandler.yAxis.maximum - dragHandler.yAxis.minimum)
        onYChanged: if (!programmatic) root.value = root.maximumValue - (knob.y - dragHandler.yAxis.minimum) * multiplier
        transformOrigin: Item.Center
        function setValue(value) { knob.y = dragHandler.yAxis.maximum - value / knob.multiplier }
        DragHandler {
            id: dragHandler
            objectName: root.objectName + " DragHandler"
            xAxis.enabled: false
            yAxis.minimum: slot.y
            yAxis.maximum: slot.height + slot.y - knob.height
        }
        TapHandler {
            id: tap
            objectName: root.objectName + " TapHandler"
            gesturePolicy: TapHandler.DragThreshold
            onTapped: {
                tapFlash.start()
                root.tapped
            }
        }
    }

    Text {
        color: "red"
        anchors.top: slot.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.value
    }

    Text {
        id: label
        font.pointSize: 9
        color: "red"
        anchors.bottom: slot.top
        anchors.bottomMargin: 5
        anchors.horizontalCenter: parent.horizontalCenter
        verticalAlignment: Text.AlignBottom
    }

    Component.onCompleted: {
        knob.programmatic = true
        knob.setValue(root.value)
        knob.programmatic = false
    }
}
