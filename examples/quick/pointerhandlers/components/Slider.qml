// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.animation

Item {
    id: root
    property int value: 50
    property int maximumValue: 99
    property alias label: label.text
    property alias tapEnabled: tap.enabled
    property alias pressed: tap.pressed
    signal tapped

    DragHandler {
        id: dragHandler
        objectName: label.text + " DragHandler"
        target: knob
        xAxis.enabled: false
    }

    WheelHandler {
        id: wheelHandler
        objectName: label.text + " WheelHandler"
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        invertible: false   // Don't let the system "natural scrolling" setting affect this
        rotationScale: -0.5 // But make it go consistently in the same direction as the fingers or wheel, a bit slow
        target: knob
        property: "y"
    }

    Rectangle {
        id: slot
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 10
        anchors.topMargin: label.height + 6
        anchors.bottomMargin: valueLabel.height + 4
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
        opacity: tap.pressed || tapFlash.running ? 1 : 0
        FlashAnimation on visible {
            id: tapFlash
        }
    }
    Image {
        id: knob
        source: "images/mixer-knob.png"
        antialiasing: true
        x: slot.x - width / 2 + slot.width / 2
        height: root.width / 2
        width: implicitWidth / implicitHeight * height
        property bool programmatic: false
        property real multiplier: root.maximumValue / (ybr.maximum - ybr.minimum)
        onYChanged: if (!programmatic) root.value = root.maximumValue - (knob.y - ybr.minimum) * multiplier
        transformOrigin: Item.Center
        function setValue(value) { knob.y = ybr.maximum - value / knob.multiplier }
        TapHandler {
            id: tap
            objectName: label.text + " TapHandler"
            gesturePolicy: TapHandler.DragThreshold
            onTapped: {
                tapFlash.start()
                root.tapped
            }
        }
        BoundaryRule on y {
            id: ybr
            minimum: slot.y
            maximum: slot.height + slot.y - knob.height
        }
    }

    Text {
        id: valueLabel
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
