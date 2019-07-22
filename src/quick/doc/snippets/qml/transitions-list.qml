/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
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

import QtQuick 2.13

Rectangle {
    id: page
    width: 640
    height: 500

    Image {
        id: userIcon
        x: topLeftRect.x
        y: topLeftRect.y

        source: "../../images/declarative-qtlogo.png"
    }

    Rectangle {
        id: topLeftRect
        anchors { left: parent.left; top: parent.top; leftMargin: 10; topMargin: 20 }
        width: 46; height: 54
        color: "Transparent"; border.color: "Gray"; radius: 6
        // Clicking here sets state to default state, returning image to initial position
        TapHandler { onTapped: page.state = 'start' }
    }

    Rectangle {
        id: middleRightRect
        anchors { right: parent.right; verticalCenter: parent.verticalCenter; rightMargin: 20 }
        width: 46; height: 54
        color: "Transparent"; border.color: "Gray"; radius: 6
        // Clicking in here sets the state to 'middleRight'
        TapHandler { onTapped: page.state = 'middleRight' }
    }

    Rectangle {
        id: bottomLeftRect
        anchors { left: parent.left; bottom: parent.bottom; leftMargin: 10; bottomMargin: 20 }
        width: 46; height: 54
        color: "Transparent"; border.color: "Gray"; radius: 6
        // Clicking in here sets the state to 'bottomLeft'
        TapHandler { onTapped: page.state = 'bottomLeft' }
    }

    states: [
        State {
            name: "start"
            PropertyChanges {
                target: userIcon
                explicit: true
                x: topLeftRect.x
                y: topLeftRect.y
            }
        },
        State {
            name: "middleRight"
            PropertyChanges {
                target: userIcon
                explicit: true
                x: middleRightRect.x
                y: middleRightRect.y
            }
        },
        State {
            name: "bottomLeft"
            PropertyChanges {
                target: userIcon
                explicit: true
                x: bottomLeftRect.x
                y: bottomLeftRect.y
            }
        }
    ]
//! [list of transitions]
    transitions: [
        Transition {
            from: "*"; to: "middleRight"
            NumberAnimation {
                properties: "x,y";
                easing.type: Easing.InOutQuad;
                duration: 2000;
            }
        },
        Transition {
            from: "*"; to: "bottomLeft";
            NumberAnimation {
                properties: "x,y";
                easing.type: Easing.InOutQuad;
                duration: 200;
            }
        },
        //If any other rectangle is clicked, the icon will return
        //to the start position at a slow speed and bounce.
        Transition {
            from: "*"; to: "*";
            NumberAnimation {
                easing.type: Easing.OutBounce;
                properties: "x,y";
                duration: 4000;
            }
        }
    ]
//! [list of transitions]
}
