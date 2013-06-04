/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
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

import QtQuick 2.0
import "content"
import "content/calculator.js" as CalcEngine


Rectangle {
    id: window
    width: 320
    height: 480
    focus: true
    color: "#272822"

    onWidthChanged: controller.reload()

    function operatorPressed(operator) { CalcEngine.operatorPressed(operator) }
    function digitPressed(digit) { CalcEngine.digitPressed(digit) }

    Item {
        id: pad
        width: 180
        NumberPad { y: 10; anchors.horizontalCenter: parent.horizontalCenter }
    }

    AnimationController {
        id: controller
        animation: ParallelAnimation {
            id: anim
            NumberAnimation { target: display; property: "x"; duration: 400; from: -16; to: window.width - display.width; easing.type: Easing.InOutQuad }
            NumberAnimation { target: pad; property: "x"; duration: 400; from: window.width - pad.width; to: 0; easing.type: Easing.InOutQuad }
            SequentialAnimation {
                NumberAnimation { target: pad; property: "scale"; duration: 200; from: 1; to: 0.97; easing.type: Easing.InOutQuad }
                NumberAnimation { target: pad; property: "scale"; duration: 200; from: 0.97; to: 1; easing.type: Easing.InOutQuad }
            }
        }
    }

    Display {
        id: display
        x: -16
        width: window.width - pad.width
        height: parent.height

        MouseArea {
            property real startX: 0
            property real oldP: 0
            property bool rewind: false

            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
            }
            height: 50
            onPositionChanged: {
                var reverse = startX > window.width / 2
                var mx = mapToItem(window, mouse.x).x
                var p = Math.abs((mx - startX) / (window.width - display.width))
                if (p < oldP)
                    rewind = reverse ? false : true
                else
                    rewind = reverse ? true : false
                controller.progress = reverse ? 1 - p : p
                oldP = p
            }
            onPressed: startX = mapToItem(window, mouse.x).x
            onReleased: {
                if (rewind)
                    controller.completeToBeginning()
                else
                    controller.completeToEnd()
            }
        }
    }

}
