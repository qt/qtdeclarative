// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import "components"

Item {
    width: 800
    height: 480

    Rectangle {
        id: rect
        anchors.fill: parent; anchors.margins: 40; anchors.topMargin: 60
        border.width: 3; border.color: "transparent"
        color: handler.pressed ? "lightsteelblue" : "darkgrey"

        TapHandler {
            id: handler
            acceptedButtons: (leftAllowedCB.checked ? Qt.LeftButton : Qt.NoButton) |
                             (middleAllowedCB.checked ? Qt.MiddleButton : Qt.NoButton) |
                             (rightAllowedCB.checked ? Qt.RightButton : Qt.NoButton)
            gesturePolicy: (policyDragThresholdCB.checked ? TapHandler.DragThreshold :
                            policyWithinBoundsCB.checked ? TapHandler.WithinBounds :
                            policyDragWithinBoundsCB.checked ? TapHandler.DragWithinBounds :
                            TapHandler.ReleaseWithinBounds)

            onCanceled: function(point) {
                console.log("canceled @ " + point.position)
                borderBlink.blinkColor = "red"
                borderBlink.start()
            }
            onTapped: function(point, button) {
                console.log("tapped button " + button + " @ " + point.scenePosition +
                            " on device '" + point.device.name + "' with modifiers " + handler.point.modifiers +
                            " " + (tapCount > 1 ? (tapCount + " times") : "for the first time"))
                if (tapCount > 1) {
                    tapCountLabel.text = tapCount
                    flashAnimation.start()
                } else {
                    borderBlink.tapFeedback(button)
                }
            }
            onLongPressed: longPressFeedback.createObject(rect,
                {"x": point.position.x, "y": point.position.y,
                 "text": "long press after\n" + handler.timeHeld.toFixed(3) + " sec",
                 "color": buttonToBlinkColor(point.pressedButtons)})
        }

        Text {
            id: tapCountLabel
            anchors.centerIn: parent
            font.pixelSize: 72
            font.weight: Font.Black
            SequentialAnimation {
                id: flashAnimation
                PropertyAction { target: tapCountLabel; property: "visible"; value: true }
                PropertyAction { target: tapCountLabel; property: "opacity"; value: 1.0 }
                PropertyAction { target: tapCountLabel; property: "scale"; value: 1.0 }
                ParallelAnimation {
                    NumberAnimation {
                        target: tapCountLabel
                        property: "opacity"
                        to: 0
                        duration: 500
                    }
                    NumberAnimation {
                        target: tapCountLabel
                        property: "scale"
                        to: 1.5
                        duration: 500
                    }
                }
            }
        }

        Rectangle {
            id: expandingCircle
            radius: handler.timeHeld * 100
            visible: radius > 0 && handler.pressed
            border.width: 3
            border.color: buttonToBlinkColor(handler.point.pressedButtons)
            color: "transparent"
            width: radius * 2
            height: radius * 2
            x: handler.point.pressPosition.x - radius
            y: handler.point.pressPosition.y - radius
            opacity: 0.25
        }

        Component {
            id: longPressFeedback
            Text { }
        }

        SequentialAnimation {
            id: borderBlink
            property color blinkColor: "red"
            function tapFeedback(button) {
                blinkColor = buttonToBlinkColor(button);
                start();
            }
            loops: 3
            ScriptAction { script: rect.border.color = borderBlink.blinkColor }
            PauseAnimation { duration: 100 }
            ScriptAction { script: rect.border.color = "transparent" }
            PauseAnimation { duration: 100 }
        }

        Text {
            text: "tap, click with different buttons, double-click, long press in this area"
            anchors {
                bottom: parent.bottom
                horizontalCenter: parent.horizontalCenter
                margins: 6
            }
        }
    }

    function buttonToBlinkColor(button) {
        switch (button) {
        case Qt.MiddleButton: return "orange";
        case Qt.RightButton:  return "magenta";
        default:              return "green";
        }
    }

    GridLayout {
        columnSpacing: 6; rowSpacing: 6
        Text {
            text: "accepted mouse clicks:"
        }
        CheckBox {
            id: leftAllowedCB
            checked: true
            text: "left"
        }
        CheckBox {
            id: middleAllowedCB
            text: "middle"
        }
        CheckBox {
            id: rightAllowedCB
            text: "right"
        }

        Text {
            text: "gesture policy:"
            horizontalAlignment: Text.AlignRight
            Layout.row: 1
            Layout.fillWidth: true
        }
        CheckBox {
            id: policyDragThresholdCB
            text: "drag threshold"
            onCheckedChanged: if (checked) {
                policyWithinBoundsCB.checked = false;
                policyReleaseWithinBoundsCB.checked = false;
                policyDragWithinBoundsCB.checked = false;
            }
        }
        CheckBox {
            id: policyWithinBoundsCB
            text: "within bounds"
            onCheckedChanged: if (checked) {
                policyDragThresholdCB.checked = false;
                policyReleaseWithinBoundsCB.checked = false;
                policyDragWithinBoundsCB.checked = false;
            }
        }
        CheckBox {
            id: policyReleaseWithinBoundsCB
            checked: true
            text: "release within bounds"
            onCheckedChanged: if (checked) {
                policyDragThresholdCB.checked = false;
                policyWithinBoundsCB.checked = false;
                policyDragWithinBoundsCB.checked = false;
            }
        }
        CheckBox {
            id: policyDragWithinBoundsCB
            text: "drag within bounds"
            onCheckedChanged: if (checked) {
                policyDragThresholdCB.checked = false;
                policyWithinBoundsCB.checked = false;
                policyReleaseWithinBoundsCB.checked = false;
            }
        }
    }
}
