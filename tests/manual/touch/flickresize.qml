// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    width: 640
    height: 360
    color: "gray"

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: 500
        contentHeight: 500

        PinchArea {
            width: Math.max(flick.contentWidth, flick.width)
            height: Math.max(flick.contentHeight, flick.height)

            property real initialWidth
            property real initialHeight
            //![0]
            onPinchStarted: {
                initialWidth = flick.contentWidth
                initialHeight = flick.contentHeight
            }

            onPinchUpdated: (pinch)=> {
                // adjust content pos due to drag
                flick.contentX += pinch.previousCenter.x - pinch.center.x
                flick.contentY += pinch.previousCenter.y - pinch.center.y

                // resize content
                flick.resizeContent(initialWidth * pinch.scale, initialHeight * pinch.scale, pinch.center)
            }

            onPinchFinished: {
                // Move its content within bounds.
                flick.returnToBounds()
            }
            //![0]

            Rectangle {
                width: flick.contentWidth
                height: flick.contentHeight
                color: "white"
                Image {
                    anchors.fill: parent
                    source: "qt-logo.jpg"
                    MouseArea {
                        anchors.fill: parent
                        onDoubleClicked: {
                            flick.contentWidth = 500
                            flick.contentHeight = 500
                        }
                    }
                }
            }
        }
    }
}
