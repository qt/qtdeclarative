/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick
import "components"

Item {
    width: 800
    height: 480

    Rectangle {
        id: rect
        anchors.fill: parent; anchors.margins: 40
        border.width: 3; border.color: "transparent"
        color: handler.pressed ? "lightsteelblue" : "darkgrey"

        TapHandler {
            id: handler
            acceptedButtons: (leftAllowedCB.checked ? Qt.LeftButton : Qt.NoButton) |
                             (middleAllowedCB.checked ? Qt.MiddleButton : Qt.NoButton) |
                             (rightAllowedCB.checked ? Qt.RightButton : Qt.NoButton)
            gesturePolicy: (policyDragThresholdCB.checked ? TapHandler.DragThreshold :
                            policyWithinBoundsCB.checked ? TapHandler.WithinBounds :
                            TapHandler.ReleaseWithinBounds)

            onCanceled: {
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

    Row {
        spacing: 6
        Text {
            text: "accepted mouse clicks:"
            anchors.verticalCenter: leftAllowedCB.verticalCenter
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
            text: "      gesture policy:"
            anchors.verticalCenter: leftAllowedCB.verticalCenter
        }
        CheckBox {
            id: policyDragThresholdCB
            text: "drag threshold"
            onCheckedChanged: if (checked) {
                policyWithinBoundsCB.checked = false;
                policyReleaseWithinBoundsCB.checked = false;
            }
        }
        CheckBox {
            id: policyWithinBoundsCB
            text: "within bounds"
            onCheckedChanged: if (checked) {
                policyDragThresholdCB.checked = false;
                policyReleaseWithinBoundsCB.checked = false;
            }
        }
        CheckBox {
            id: policyReleaseWithinBoundsCB
            checked: true
            text: "release within bounds"
            onCheckedChanged: if (checked) {
                policyDragThresholdCB.checked = false;
                policyWithinBoundsCB.checked = false;
            }
        }
    }
}
