/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

import QtQuick 2.2
import QtGraphicalEffects 1.0

Rectangle {
    id: delegate

    property bool selected: ListView.isCurrentItem
    property real itemSize
    width: itemSize
    height: itemSize

    Image {
        anchors.centerIn: parent
        source: image
    }

    Item {
        id: title
        anchors.fill: parent

        Text {
            id: titleText

            anchors {
                left: parent.left; leftMargin: 20
                right: parent.right; rightMargin: 20
                top: parent.top; topMargin: 20
            }
            font { pixelSize: 18; bold: true }
            text: name
            color: selected ? "#ffffff" : "#ebebdd"
            Behavior on color { ColorAnimation { duration: 150 } }
        }

        DropShadow {
            source: titleText
            anchors.fill: titleText
            horizontalOffset: selected ? 3 : 1
            verticalOffset: selected ? 3 : 1
            radius: 4
            color: "#2f1000"
            samples: 8

            Behavior on horizontalOffset { NumberAnimation { duration: 300 } }
            Behavior on verticalOffset   { NumberAnimation { duration: 300 } }
        }

        states: [
            State {
                name: "selected"
                when: selected
                PropertyChanges { target: title; scale: "1.1" }
            }]

        transitions: [
            Transition {
                to: "selected"
                SequentialAnimation {
                    id: titleAnimation
                    PropertyAnimation { target: title; property: "scale"; duration: 300 }
                }
            },
            Transition {
                to: ""
                animations: titleAnimation
            }]
    }

    BusyIndicator {
        scale: 0.8
        visible: delegate.ListView.isCurrentItem && window.loading
        anchors.centerIn: parent
    }

    MouseArea {
        anchors.fill: delegate
        onClicked: {
            delegate.ListView.view.currentIndex = index
            if (window.currentFeed == feed)
                feedModel.reload()
            else
                window.currentFeed = feed
        }
    }
}
