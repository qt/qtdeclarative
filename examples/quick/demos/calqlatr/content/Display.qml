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

Item {
    id: display
    property bool enteringDigits: false

    function displayOperator(operator)
    {
        listView.model.append({ "operator": operator, "operand": "" })
        enteringDigits = true
    }

    function newLine(operator, operand)
    {
        listView.model.append({ "operator": operator, "operand": operand })
        enteringDigits = false
        listView.positionViewAtEnd()
    }

    function appendDigit(digit)
    {
        if (!enteringDigits)
            listView.model.append({ "operator": "", "operand": "" })
        var i = listView.model.count - 1;
        listView.model.get(i).operand = listView.model.get(i).operand + digit;
        enteringDigits = true
    }

    function clear()
    {
        if (enteringDigits) {
            var i = listView.model.count - 1
            if (i >= 0)
                listView.model.remove(i)
            enteringDigits = false
        }
    }

    Item {
        id: theItem
        width: parent.width + 32
        height: parent.height

        Rectangle {
            id: rect
            x: 16
            color: "white"
            height: parent.height
            width: display.width - 16
        }
        Image {
            anchors.right: rect.left
            source: "images/paper-edge-left.png"
            height: parent.height
            fillMode: Image.TileVertically
        }
        Image {
            anchors.left: rect.right
            source: "images/paper-edge-right.png"
            height: parent.height
            fillMode: Image.TileVertically
        }

        Image {
            id: grip
            source: "images/paper-grip.png"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20
        }

        ListView {
            id: listView
            x: 16; y: 30
            width: display.width
            height: display.height - 50 - y
            delegate: Item {
                height: 20
                width: parent.width
                Text {
                    id: operator
                    x: 8
                    font.pixelSize: 18
                    color: "#6da43d"
                    text: model.operator
                }
                Text {
                    id: operand
                    font.pixelSize: 18
                    anchors.right: parent.right
                    anchors.rightMargin: 26
                    text: model.operand
                }
            }
            model: ListModel { }
        }

    }

}
