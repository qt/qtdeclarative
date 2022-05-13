// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "components"

Rectangle {
    id: root
    color: "#444"
    width: 480
    height: 640

    FakeFlickable {
        id: ff
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: rightSB.width

        Text {
            id: text
            color: "beige"
            font.family: "mono"
            font.pointSize: slider.value
            onTextChanged: console.log("text geom " + width + "x" + height +
                                       ", parent " + parent + " geom " + parent.width + "x" + parent.height)
        }

        onFlickStarted: {
            root.border.color = "green"
            console.log("flick started with velocity " + velocity)
        }
        onFlickEnded: {
            root.border.color = "transparent"
            console.log("flick ended with velocity " + velocity)
        }

        Component.onCompleted: {
            var request = new XMLHttpRequest()
            request.open('GET', 'components/FakeFlickable.qml')
            request.onreadystatechange = function(event) {
                if (request.readyState === XMLHttpRequest.DONE)
                    text.text = request.responseText
            }
            request.send()
        }
    }

    ScrollBar {
        id: rightSB
        objectName: "rightSB"
        flick: ff
        height: parent.height - width
        anchors.right: parent.right
    }

    ScrollBar {
        id: bottomSB
        objectName: "bottomSB"
        flick: ff
        width: parent.width - height
        anchors.bottom: parent.bottom
    }

    Rectangle {
        id: cornerCover
        color: "lightgray"
        width: rightSB.width
        height: bottomSB.height
        anchors {
            right: parent.right
            bottom: parent.bottom
        }
    }

    LeftDrawer {
        width: buttonRow.implicitWidth + 20
        anchors.verticalCenter: parent.verticalCenter
        Column {
            anchors.fill: parent
            anchors.margins: 10
            Slider {
                id: slider
                width: parent.width
                height: parent.height - buttonRow.implicitHeight
                label: "font\nsize"
                maximumValue: 36
                value: 14
            }
            Row {
                id: buttonRow
                spacing: 4
                Button {
                    text: "⭯"
                    onTapped: ff.rotation -= 45
                }
                Button {
                    text: "⭮"
                    onTapped: ff.rotation += 45
                }
            }
        }
    }
}
