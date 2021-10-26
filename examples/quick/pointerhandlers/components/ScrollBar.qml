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

Rectangle {
    id: root
    property real contentHeight: 100
    property FakeFlickable flick: null
    property real position

    onPositionChanged: if (flick && (drag.active || tap.active)) {
        if (knob.state === "horizontal")
            flick.contentX = position * knob.scrollDistance
        else if (knob.state === "vertical")
            flick.contentY = position * knob.scrollDistance
    }

    color: palette.button
    border.color: Qt.darker(palette.button, 1.5)
    gradient: Gradient {
        GradientStop { position: 0; color: Qt.darker(palette.button, 1.3) }
        GradientStop { position: 1; color: palette.button }
    }
    antialiasing: true
    radius: Math.min(width, height) / 6
    width: 20
    height: 20

    TapHandler {
        id: tap
        onTapped: {
            if (knob.state === "horizontal")
                knob.x = position.x - knob.width / 2
            else if (knob.state === "vertical")
                knob.y = position.y - knob.height / 2
        }
    }

    Rectangle {
        id: knob
        border.color: "black"
        border.width: 1
        gradient: Gradient {
            GradientStop { position: 0; color: palette.button }
            GradientStop { position: 1; color: Qt.darker(palette.button, 1.3) }
        }
        radius: 2
        antialiasing: true
        state: root.height > root.width ? "vertical" : root.height < root.width ? "horizontal" : ""
        property real scrollDistance: 0
        property real scrolledX: 0
        property real scrolledY: 0
        property real max: 0

        Binding on x {
            value: knob.scrolledX
            when: !drag.active
        }

        Binding on y {
            value: knob.scrolledY
            when: !drag.active
        }

        states: [
            // We will control the horizontal. We will control the vertical.
            State {
                name: "horizontal"
                PropertyChanges {
                    target: knob
                    max: root.width - knob.width
                    scrolledX: Math.min(max, Math.max(0, max * flick.contentX / (flick.width - flick.contentWidth)))
                    scrolledY: 1
                    scrollDistance: flick.width - flick.contentWidth
                    width: flick.width * (flick.width / flick.contentWidth) - (height - anchors.margins) * 2
                    height: root.height - 2
                }
                PropertyChanges {
                    target: drag
                    xAxis.minimum: 0
                    xAxis.maximum: knob.max
                    yAxis.minimum: 1
                    yAxis.maximum: 1
                }
                PropertyChanges {
                    target: root
                    position: knob.x / drag.xAxis.maximum
                }
            },
            State {
                name: "vertical"
                PropertyChanges {
                    target: knob
                    max: root.height - knob.height
                    scrolledX: 1
                    scrolledY: Math.min(max, Math.max(0, max * flick.contentY / (flick.height - flick.contentHeight)))
                    scrollDistance: flick.height - flick.contentHeight
                    width: root.width - 2
                    height: root.width - 2
                }
                PropertyChanges {
                    target: drag
                    xAxis.minimum: 1
                    xAxis.maximum: 1
                    yAxis.minimum: 0
                    yAxis.maximum: knob.max
                }
                PropertyChanges {
                    target: root
                    position: knob.y / drag.yAxis.maximum
                }
            }
        ]

        DragHandler {
            id: drag
        }
    }
}
