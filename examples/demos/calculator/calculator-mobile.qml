/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtQuick.Window 2.0
import "content"
import "content/calculator.js" as CalcEngine

Rectangle {
    id: window

    width: 320; height: 480
    color: "#282828"

    property string rotateLeft: "\u2939"
    property string rotateRight: "\u2935"
    property string leftArrow: "\u2190"
    property string division : "\u00f7"
    property string multiplication : "\u00d7"
    property string squareRoot : "\u221a"
    property string plusminus : "\u00b1"

    function doOp(operation) { CalcEngine.doOperation(operation) }

    Item {
        id: main
        state: "orientation " + Screen.orientation

        //Note: Assumes a primarily portrait device
        property bool landscapeWindow: window.width > window.height 
        property real baseWidth: landscapeWindow ? window.height : window.width
        property real baseHeight: landscapeWindow ? window.width : window.height
        property real rotationDelta: landscapeWindow ? -90 : 0

        rotation: rotationDelta
        width: main.baseWidth
        height: main.baseHeight
        anchors.centerIn: parent

        Column {
            id: box; spacing: 8

            anchors { fill: parent; topMargin: 6; bottomMargin: 6; leftMargin: 6; rightMargin: 6 }

            Display {
                id: display
                width: box.width-3
                height: 64
            }

            Column {
                id: column; spacing: 6

                property real h: ((box.height - 72) / 6) - ((spacing * (6 - 1)) / 6)
                property real w: (box.width / 4) - ((spacing * (4 - 1)) / 4)

                Row {
                    spacing: 6
                    Button { width: column.w; height: column.h; color: 'purple'; operation: "Off" }
                    Button { width: column.w; height: column.h; color: 'purple'; operation: window.leftArrow }
                    Button { width: column.w; height: column.h; color: 'purple'; operation: "C" }
                    Button { width: column.w; height: column.h; color: 'purple'; operation: "AC" }
                }

                Row {
                    spacing: 6
                    property real w: (box.width / 4) - ((spacing * (4 - 1)) / 4)

                    Button { width: column.w; height: column.h; color: 'green'; operation: "mc" }
                    Button { width: column.w; height: column.h; color: 'green'; operation: "m+" }
                    Button { width: column.w; height: column.h; color: 'green'; operation: "m-" }
                    Button { width: column.w; height: column.h; color: 'green'; operation: "mr" }
                }

                Grid {
                    id: grid; rows: 5; columns: 5; spacing: 6

                    property real w: (box.width / columns) - ((spacing * (columns - 1)) / columns)

                    Button { width: grid.w; height: column.h; operation: "7"; color: 'blue' }
                    Button { width: grid.w; height: column.h; operation: "8"; color: 'blue' }
                    Button { width: grid.w; height: column.h; operation: "9"; color: 'blue' }
                    Button { width: grid.w; height: column.h; operation: window.division }
                    Button { width: grid.w; height: column.h; operation: window.squareRoot }
                    Button { width: grid.w; height: column.h; operation: "4"; color: 'blue' }
                    Button { width: grid.w; height: column.h; operation: "5"; color: 'blue' }
                    Button { width: grid.w; height: column.h; operation: "6"; color: 'blue' }
                    Button { width: grid.w; height: column.h; operation: window.multiplication }
                    Button { width: grid.w; height: column.h; operation: "x^2" }
                    Button { width: grid.w; height: column.h; operation: "1"; color: 'blue' }
                    Button { width: grid.w; height: column.h; operation: "2"; color: 'blue' }
                    Button { width: grid.w; height: column.h; operation: "3"; color: 'blue' }
                    Button { width: grid.w; height: column.h; operation: "-" }
                    Button { width: grid.w; height: column.h; operation: "1/x" }
                    Button { width: grid.w; height: column.h; operation: "0"; color: 'blue' }
                    Button { width: grid.w; height: column.h; operation: "." }
                    Button { width: grid.w; height: column.h; operation: window.plusminus }
                    Button { width: grid.w; height: column.h; operation: "+" }
                    Button { width: grid.w; height: column.h; operation: "="; color: 'red' }
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
