/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtQuick.Window 2.0 as Window

Item {
    id: root
    width: 800
    height: dpi + dpcm
    property real dpcm: Window.Screen.physicalDotsPerInch / 2.54
    property real dpi: Window.Screen.physicalDotsPerInch
    Item {
        id: main
        state: "orientation " + Window.Screen.currentOrientation

        property bool landscapeWindow: Window.Screen.primaryOrientation == Qt.LandscapeOrientation
        property real baseWidth: landscapeWindow ? root.height : root.width
        property real baseHeight: landscapeWindow ? root.width : root.height
        property real rotationDelta: landscapeWindow ? -90 : 0

        rotation: rotationDelta
        width: main.baseWidth
        height: main.baseHeight
        anchors.centerIn: parent

        Repeater {
            model: Math.ceil(main.width/ dpcm) + 1
            delegate: Rectangle{
                border.width: 1
                color: "goldenrod"
                width: dpcm
                height: dpcm
                x: dpcm * (index - 1)
                Text {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 2
                    font.pointSize: 6
                    text: index + " cm"
                }
            }
        }

        Repeater {
            model: Math.ceil(main.width / dpi) + 1
            delegate: Rectangle{
                border.width: 1
                color: "goldenrod"
                width: dpi
                height: dpi
                x: dpi * (index - 1)
                y: dpcm
                Text {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 2
                    font.pointSize: 8
                    text: index + " in"
                }
            }
        }

        states: [
            State {
                name: "orientation " + Qt.LandscapeOrientation
                PropertyChanges { target: main; rotation: 90 + rotationDelta; width: main.baseHeight; height: main.baseWidth }
            },
            State {
                name: "orientation " + Qt.InvertedPortraitOrientation
                PropertyChanges { target: main; rotation: 180 + rotationDelta; }
            },
            State {
                name: "orientation " + Qt.InvertedLandscapeOrientation
                PropertyChanges { target: main; rotation: 270 + rotationDelta; width: main.baseHeight; height: main.baseWidth }
            }
        ]

        transitions: Transition {
            SequentialAnimation {
                RotationAnimation { direction: RotationAnimation.Shortest; duration: 300; easing.type: Easing.InOutQuint  }
                NumberAnimation { properties: "x,y,width,height"; duration: 300; easing.type: Easing.InOutQuint }
            }
        }
    }
}
