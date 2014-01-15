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

Rectangle {
    id: page
    width: 320; height: 480
    property real effectiveOpacity: 1.0

    Timer {
        interval: 2000
        running: true
        repeat: true
        onTriggered: effectiveOpacity = (effectiveOpacity == 1.0 ? 0.0 : 1.0);
    }

    Column {
        y: 0

        populate: Transition {
            NumberAnimation { properties: "x,y"; from: 200; duration: 1500; easing.type: Easing.OutBounce }
        }
        add: Transition {
            NumberAnimation { properties: "y"; easing.type: Easing.OutQuad }
        }
        move: Transition {
            NumberAnimation { properties: "y"; easing.type: Easing.OutBounce }
        }

        Rectangle { color: "red"; width: 100; height: 50; border.color: "black"; radius: 15 }

        Rectangle {
            id: blueV1
            visible: opacity != 0
            width: 100; height: 50
            color: "lightsteelblue"
            border.color: "black"
            radius: 15
            Behavior on opacity { NumberAnimation {} }
            opacity: effectiveOpacity
        }

        Rectangle { color: "green"; width: 100; height: 50; border.color: "black"; radius: 15 }

        Rectangle {
            id: blueV2
            visible: opacity != 0
            width: 100; height: 50
            color: "lightsteelblue"
            border.color: "black"
            radius: 15
            Behavior on opacity { NumberAnimation {} }
            opacity: effectiveOpacity
        }

        Rectangle { color: "orange"; width: 100; height: 50; border.color: "black"; radius: 15 }
        Rectangle { color: "red"; width: 100; height: 50; border.color: "black"; radius: 15 }
    }

    Row {
        y: 320

        populate: Transition {
            NumberAnimation { properties: "x,y"; from: 200; duration: 1500; easing.type: Easing.OutBounce }
        }
        add: Transition {
            NumberAnimation { properties: "x"; easing.type: Easing.OutQuad }
        }
        move: Transition {
            NumberAnimation { properties: "x"; easing.type: Easing.OutBounce }
        }

        Rectangle { color: "red"; width: 50; height: 100; border.color: "black"; radius: 15 }

        Rectangle {
            id: blueH1
            visible: opacity != 0
            width: 50; height: 100
            color: "lightsteelblue"
            border.color: "black"
            radius: 15
            Behavior on opacity { NumberAnimation {} }
            opacity: effectiveOpacity
        }

        Rectangle { color: "green"; width: 50; height: 100; border.color: "black"; radius: 15 }

        Rectangle {
            id: blueH2
            visible: opacity != 0
            width: 50; height: 100
            color: "lightsteelblue"
            border.color: "black"
            radius: 15
            Behavior on opacity { NumberAnimation {} }
            opacity: effectiveOpacity
        }

        Rectangle { color: "orange"; width: 50; height: 100; border.color: "black"; radius: 15 }
        Rectangle { color: "red"; width: 50; height: 100; border.color: "black"; radius: 15 }
    }

    Grid {
        x: 120; y: 0
        columns: 3

        populate: Transition {
            NumberAnimation { properties: "x,y"; from: 200; duration: 1500; easing.type: Easing.OutBounce }
        }
        add: Transition {
            NumberAnimation { properties: "x,y"; easing.type: Easing.OutBounce }
        }
        move: Transition {
            NumberAnimation { properties: "x,y"; easing.type: Easing.OutBounce }
        }


        Rectangle { color: "red"; width: 50; height: 50; border.color: "black"; radius: 15 }

        Rectangle {
            id: blueG1
            visible: opacity != 0
            width: 50; height: 50
            color: "lightsteelblue"
            border.color: "black"
            radius: 15
            Behavior on opacity { NumberAnimation {} }
            opacity: effectiveOpacity
        }

        Rectangle { color: "green"; width: 50; height: 50; border.color: "black"; radius: 15 }

        Rectangle {
            id: blueG2
            visible: opacity != 0
            width: 50; height: 50
            color: "lightsteelblue"
            border.color: "black"
            radius: 15
            Behavior on opacity { NumberAnimation {} }
            opacity: effectiveOpacity
        }

        Rectangle { color: "orange"; width: 50; height: 50; border.color: "black"; radius: 15 }

        Rectangle {
            id: blueG3
            visible: opacity != 0
            width: 50; height: 50
            color: "lightsteelblue"
            border.color: "black"
            radius: 15
            Behavior on opacity { NumberAnimation {} }
            opacity: effectiveOpacity
        }

        Rectangle { color: "red"; width: 50; height: 50; border.color: "black"; radius: 15 }
        Rectangle { color: "green"; width: 50; height: 50; border.color: "black"; radius: 15 }
        Rectangle { color: "orange"; width: 50; height: 50; border.color: "black"; radius: 15 }
    }

    Flow {
        x: 120; y: 160; width: 150

        //! [move]
        move: Transition {
            NumberAnimation { properties: "x,y"; easing.type: Easing.OutBounce }
        }
        //! [move]

        //! [add]
        add: Transition {
            NumberAnimation { properties: "x,y"; easing.type: Easing.OutBounce }
        }
        //! [add]

        //! [populate]
        populate: Transition {
            NumberAnimation { properties: "x,y"; from: 200; duration: 1500; easing.type: Easing.OutBounce }
        }
        //! [populate]

        Rectangle { color: "red"; width: 50; height: 50; border.color: "black"; radius: 15 }

        Rectangle {
            id: blueF1
            visible: opacity != 0
            width: 60; height: 50
            color: "lightsteelblue"
            border.color: "black"
            radius: 15
            Behavior on opacity { NumberAnimation {} }
            opacity: effectiveOpacity
        }

        Rectangle { color: "green"; width: 30; height: 50; border.color: "black"; radius: 15 }

        Rectangle {
            id: blueF2
            visible: opacity != 0
            width: 60; height: 50
            color: "lightsteelblue"
            border.color: "black"
            radius: 15
            Behavior on opacity { NumberAnimation {} }
            opacity: effectiveOpacity
        }

        Rectangle { color: "orange"; width: 50; height: 50; border.color: "black"; radius: 15 }

        Rectangle {
            id: blueF3
            visible: opacity != 0
            width: 40; height: 50
            color: "lightsteelblue"
            border.color: "black"
            radius: 15
            Behavior on opacity { NumberAnimation {} }
            opacity: effectiveOpacity
        }

        Rectangle { color: "red"; width: 80; height: 50; border.color: "black"; radius: 15 }
    }
}
