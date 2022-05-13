// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "qrc:/quick/shared/" as Examples

Rectangle {
    id: root

    Item {
        id: flickArea
        anchors.fill: parent
        anchors.bottomMargin: bottomFlow.implicitHeight + 4
        clip: true

        Flickable {
            id: flick
            anchors.fill: parent
            anchors.margins: 6
            contentWidth: text.implicitWidth
            contentHeight: text.implicitHeight
            pixelAligned: pxAlignCB.checked
            synchronousDrag: syncDragCB.checked
            Text {
                id: text
                text: "foo bar"
                font.family: "mono"
            }
            onContentXChanged: canvas.requestPaint()
            onContentYChanged: canvas.requestPaint()
            maximumFlickVelocity: maxVelocitySlider.value
            flickDeceleration: decelSlider.value
        }

        Timer { id: fadeTimer; interval: 1000; onTriggered: { hfade.start(); } }

        MouseArea {
            id: verticalScrollArea
            anchors {
                right: parent.right
                top: flick.top
                bottom: flick.bottom
            }
            width: 36
            onMouseYChanged: {
                var contentY = Math.min(height, mouseY) / height * (flick.contentHeight - height)
                flick.contentY = Math.max(0, contentY)
            }
            onReleased: flick.returnToBounds()
            Rectangle {
                anchors.right: parent.right
                anchors.margins: 2
                color: "darkgrey"
                width: 20
                radius: 2
                antialiasing: true
                height: flick.height * (flick.height / flick.contentHeight) - anchors.margins * 2
                y: flick.contentY * (flick.height / flick.contentHeight)

                Rectangle {
                    anchors.top: parent.top
                    width: parent.width
                    height: 6
                    radius: 2
                    color: "blue"
                    visible: flick.atYBeginning
                }

                Rectangle {
                    anchors.top: parent.bottom
                    width: parent.width
                    height: 6
                    radius: 2
                    color: "blue"
                    visible: flick.atYEnd
                }

                Text {
                    anchors.centerIn: parent
                    text: flick.contentY.toFixed(2)
                    rotation: 90
                    style: Text.Outline
                    styleColor: "white"
                    color: "black"
                }

            }
        }

        Rectangle {
            id: horizontalScrollDecorator
            anchors.bottom: flick.bottom
            anchors.bottomMargin: -4
            color: "darkgrey"
            border.color: "black"
            border.width: 1
            height: 5
            radius: 2
            antialiasing: true
            width: flick.width * (flick.width / flick.contentWidth) - (height - anchors.margins) * 2
            x:  flick.contentX * (flick.width / flick.contentWidth)
            NumberAnimation on opacity { id: hfade; to: 0; duration: 500 }
            onXChanged: { opacity = 1.0; fadeTimer.restart() }
        }

        Canvas {
            id: canvas
            anchors.fill: parent
            antialiasing: true
            renderTarget: Canvas.FramebufferObject
            onPaint: {
                var ctx = canvas.getContext('2d');
                ctx.save()
                ctx.clearRect(0, 0, canvas.width, canvas.height)
                ctx.strokeStyle = "green"
                ctx.fillStyle = "green"
                ctx.lineWidth = 1

                if (flick.horizontalVelocity) {
                    ctx.save()
                    ctx.beginPath()
                    ctx.translate((flick.horizontalVelocity < 0 ? width : 0), height / 2)
                    ctx.moveTo(0, 0)
                    var velScaled = flick.horizontalVelocity / 10
                    var arrowOffset = (flick.horizontalVelocity < 0 ? 10 : -10)
                    ctx.lineTo(velScaled, 0)
                    ctx.lineTo(velScaled + arrowOffset, -4)
                    ctx.lineTo(velScaled + arrowOffset, 4)
                    ctx.lineTo(velScaled, 0)
                    ctx.closePath()
                    ctx.stroke()
                    ctx.fill()
                    ctx.restore()
                }

                if (flick.verticalVelocity) {
                    ctx.save()
                    ctx.beginPath()
                    ctx.translate(width / 2, (flick.verticalVelocity < 0 ? height : 0))
                    ctx.moveTo(0, 0)
                    var velScaled = flick.verticalVelocity / 10
                    var arrowOffset = (flick.verticalVelocity < 0 ? 10 : -10)
                    ctx.lineTo(0, velScaled)
                    ctx.lineTo(-4, velScaled + arrowOffset)
                    ctx.lineTo(4, velScaled + arrowOffset)
                    ctx.lineTo(0, velScaled)
                    ctx.closePath()
                    ctx.stroke()
                    ctx.fill()
                    ctx.restore()
                }

                ctx.restore()
            }
        }
    }

    Row {
        id: bottomFlow
        anchors.bottom: parent.bottom
        width: parent.width - 12
        height: 110
        x: 6
        spacing: 12

        Item {
            id: progFlickItem
            width: progFlickRow.implicitWidth
            height: progFlickRow.implicitHeight + 4 + flickingLabel.implicitHeight
            Text { id: progLabel; text: "programmatic flick: h " + xvelSlider.value.toFixed(1) + " v " + yvelSlider.value.toFixed(1) }
            Row {
                id: progFlickRow
                y: progLabel.height
                spacing: 4

                Column {
                    Examples.Slider {
                        id: xvelSlider
                        min: -5000
                        max: 5000
                        init: 5000
                        width: 250
                        name: "X"
                        minLabelWidth: 0
                    }
                    Examples.Slider {
                        id: yvelSlider
                        min: -5000
                        max: 5000
                        init: 2500
                        width: 250
                        name: "Y"
                        minLabelWidth: 0
                    }
                }

                Grid {
                    columns: 2
                    spacing: 2
                    Examples.Button {
                        text: "flick"
                        onClicked: flick.flick(xvelSlider.value, yvelSlider.value)
                        width: zeroButton.width
                    }
                    Examples.Button {
                        text: "cancel"
                        onClicked: flick.cancelFlick()
                        width: zeroButton.width
                    }
                    Examples.Button {
                        id: zeroButton
                        text: "<- zero"
                        onClicked: {
                            xvelSlider.setValue(5000)
                            yvelSlider.setValue(5000)
                        }
                    }
                    Examples.Button {
                        text: "home"
                        width: zeroButton.width
                        onClicked: {
                            flick.contentX = 0
                            flick.contentY = 0
                        }
                    }
                }
            }
        }

        Column {
            height: parent.height
            width: movingLabel.implicitWidth * 1.5
            spacing: 2
            Text {
                id: movingLabel
                text: "moving:"
                color: flick.moving ? "green" : "black"
            }
            Rectangle {
                width: parent.width
                height: hVelLabel.implicitHeight + 4
                color: flick.movingHorizontally ? "green" : "darkgrey"
                Text {
                    id: hVelLabel
                    anchors.centerIn: parent
                    color: "white"
                    text: "h " + flick.horizontalVelocity.toFixed(2)
                }
            }
            Rectangle {
                width: parent.width
                height: vVelLabel.implicitHeight + 4
                color: flick.movingVertically ? "green" : "darkgrey"
                Text {
                    id: vVelLabel
                    anchors.centerIn: parent
                    color: "white"
                    text: "v " + flick.verticalVelocity.toFixed(2)
                }
                Examples.Slider {
                    id: maxVelocitySlider
                    name: "max vel"
                    anchors.left: parent.left
                    anchors.top: parent.bottom
                    max: 10000
                    init: 2500
                }
                Text {
                    anchors.left: maxVelocitySlider.right
                    anchors.verticalCenter: maxVelocitySlider.verticalCenter
                    text: Math.round(flick.maximumFlickVelocity)
                }
                Examples.Slider {
                    id: decelSlider
                    name: "decel"
                    anchors.right: maxVelocitySlider.right
                    anchors.top: maxVelocitySlider.bottom
                    min: 0
                    max: 10000
                    init: 1500
                }
                Text {
                    anchors.left: decelSlider.right
                    anchors.verticalCenter: decelSlider.verticalCenter
                    text: Math.round(flick.flickDeceleration)
                }
            }
        }

        Column {
            height: parent.height
            width: draggingLabel.implicitWidth
            spacing: 2
            Text {
                id: draggingLabel
                text: "dragging:"
                color: flick.dragging ? "green" : "black"
            }
            Rectangle {
                width: draggingLabel.implicitWidth
                height: hVelLabel.implicitHeight + 4
                color: flick.draggingHorizontally ? "green" : "darkgrey"
                Text {
                    anchors.centerIn: parent
                    color: "white"
                    text: "h"
                }
            }
            Rectangle {
                width: draggingLabel.implicitWidth
                height: vVelLabel.implicitHeight + 4
                color: flick.draggingVertically ? "green" : "darkgrey"
                Text {
                    anchors.centerIn: parent
                    color: "white"
                    text: "v"
                }
            }
        }

        Column {
            height: parent.height
            width: flickingLabel.implicitWidth
            spacing: 2
            Text {
                id: flickingLabel
                text: "flicking:"
                color: flick.flicking ? "green" : "black"
            }
            Rectangle {
                width: flickingLabel.implicitWidth
                height: hVelLabel.implicitHeight + 4
                color: flick.flickingHorizontally ? "green" : "darkgrey"
                Text {
                    anchors.centerIn: parent
                    color: "white"
                    text: "h"
                }
            }
            Rectangle {
                width: flickingLabel.implicitWidth
                height: vVelLabel.implicitHeight + 4
                color: flick.flickingVertically ? "green" : "darkgrey"
                Text {
                    anchors.centerIn: parent
                    color: "white"
                    text: "v"
                }
            }
        }

        Column {
            spacing: 2
            Examples.CheckBox {
                id: pxAlignCB
                text: "pixel aligned"
            }
            Examples.CheckBox {
                id: syncDragCB
                text: "synchronous drag"
            }
            Text {
                text: "content X " + flick.contentX.toFixed(2) + " Y " + flick.contentY.toFixed(2)
            }
        }

        Column {
            Row {
                spacing: 2
                Examples.Button {
                    id: decrButton
                    text: "-"
                    onClicked: flick.flickDeceleration -= 100
                    Timer {
                        running: decrButton.pressed
                        interval: 100; repeat: true
                        onTriggered: flick.flickDeceleration -= 100
                    }
                }
                Text {
                    horizontalAlignment: Text.AlignHCenter
                    text: "decel:\n" + flick.flickDeceleration.toFixed(4)
                }
                Examples.Button {
                    id: incrButton
                    text: "+"
                    onClicked: flick.flickDeceleration += 100
                }
                Timer {
                    running: incrButton.pressed
                    interval: 100; repeat: true
                    onTriggered: flick.flickDeceleration += 100
                }
            }
        }
    }

    Component.onCompleted: {
        var request = new XMLHttpRequest()
        request.open('GET', 'qrc:/flicktext.qml')
        request.onreadystatechange = function(event) {
            if (request.readyState === XMLHttpRequest.DONE)
                text.text = request.responseText
        }
        request.send()
    }
}
