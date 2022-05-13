// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12

Rectangle {
    id: root
    property alias label: label.text
    property alias pressed: tap.pressed
    property bool checked: false
    property alias gesturePolicy: tap.gesturePolicy
    property alias enabled: tap.enabled
    signal tapped

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
        onTapped: {
            tapFlash.start()
            root.tapped()
        }
    }

    Text {
        id: label
        font.pointSize: 14
        text: "Button"
        anchors.centerIn: parent
    }

    Rectangle {
        anchors.fill: parent; anchors.margins: -5
        color: "transparent"; border.color: "#4400FFFF"
        border.width: 5; radius: root.radius; antialiasing: true
        opacity: tapFlash.running ? 1 : 0
        FlashAnimation on visible { id: tapFlash }
    }

    Rectangle {
        objectName: "expandingCircle"
        radius: tap.timeHeld * 100
        visible: radius > 0 && tap.pressed
        border.width: 3
        border.color: "cyan"
        color: "transparent"
        width: radius * 2
        height: radius * 2
        x: tap.point.scenePressPosition.x - radius
        y: tap.point.scenePressPosition.y - radius
        opacity: 0.25
        Component.onCompleted: {
            // get on top of all the buttons
            var par = root.parent;
            while (par.parent)
                par = par.parent;
            parent = par;
        }
    }
}
