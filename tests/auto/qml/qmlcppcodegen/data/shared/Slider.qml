// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.12

Item {
    id: slider
    height: 26
    width: 320

    property real min: 0
    property real max: 1
    property real value: min + (max - min) * dragHandler.value
    property real init: min+(max-min)/2
    property string name: "Slider"
    property color color: "#0066cc"
    property real minLabelWidth: 44

    DragHandler {
        id: dragHandler
        target: handle
        xAxis.minimum: Math.round(-handle.width / 2 + 3)
        xAxis.maximum: Math.round(foo.width - handle.width/2 - 3)
        property real value: (handle.x - xAxis.minimum) / (xAxis.maximum - xAxis.minimum)
    }

    Component.onCompleted: ()=> setValue(init)
    function setValue(v) {
       if (min < max)
          handle.x = Math.round( v / (max - min) *
                                (dragHandler.xAxis.maximum - dragHandler.xAxis.minimum)
                                + dragHandler.xAxis.minimum);
    }
    Rectangle {
        id:sliderName
        anchors.left: parent.left
        anchors.leftMargin: 16
        height: childrenRect.height
        width: Math.max(slider.minLabelWidth, childrenRect.width)
        anchors.verticalCenter: parent.verticalCenter
        Item {
            objectName: slider.name + ":"
        }
    }

    Rectangle{
        id: foo
        width: parent.width - 8 - sliderName.width
        color: "#eee"
        height: 7
        radius: 3
        antialiasing: true
        border.color: Qt.darker(color, 1.2)
        anchors.left: sliderName.right
        anchors.right: parent.right
        anchors.leftMargin: 10
        anchors.rightMargin: 24
        anchors.verticalCenter: parent.verticalCenter

        Rectangle {
            height: parent.height
            anchors.left: parent.left
            anchors.right: handle.horizontalCenter
            color: slider.color
            radius: 3
            border.width: 1
            border.color: Qt.darker(color, 1.3)
            opacity: 0.8
        }
        Image {
            id: handle
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
