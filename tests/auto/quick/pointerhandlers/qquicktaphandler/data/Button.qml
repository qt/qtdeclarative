// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12

Rectangle {
    id: root
    property alias label: label.text
    property alias active: tap.active
    property alias pressed: tap.pressed
    property bool checked: false
    property alias gesturePolicy: tap.gesturePolicy
    property point tappedPosition: Qt.point(0, 0)
    signal tapped
    signal canceled

    width: label.implicitWidth * 1.5; height: label.implicitHeight * 2.0
    border.color: "#9f9d9a"; border.width: 1; radius: height / 4; antialiasing: true

    gradient: Gradient {
        GradientStop { position: 0.0; color: tap.pressed ? "#b8b5b2" : "#efebe7" }
        GradientStop { position: 1.0; color: "#b8b5b2" }
    }

    TapHandler {
        id: tap
        objectName: label.text
        longPressThreshold: 100 // CI can be insanely slow, so don't demand a timely release to generate onTapped
        onSingleTapped: console.log("Single tap")
        onDoubleTapped: console.log("Double tap")
        onTapped: {
            console.log("Tapped")
            tapFlash.start()
            root.tappedPosition = point.scenePosition
            root.tapped()
        }
        onCanceled: root.canceled()
    }

    Text {
        id: label
        font.pointSize: 14
        text: "Button"
        anchors.centerIn: parent
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.width: 2; radius: root.radius; antialiasing: true
        opacity: tapFlash.running ? 1 : 0
        FlashAnimation on visible { id: tapFlash }
    }

    Rectangle {
        objectName: "expandingCircle"
        radius: tap.timeHeld * 100
        visible: radius > 0 && tap.pressed
        border.width: 3
        border.color: "blue"
        color: "transparent"
        width: radius * 2
        height: radius * 2
        x: tap.point.scenePressPosition.x - radius
        y: tap.point.scenePressPosition.y - radius
        opacity: 0.25
        Component.onCompleted: parent = root.parent
    }
}
