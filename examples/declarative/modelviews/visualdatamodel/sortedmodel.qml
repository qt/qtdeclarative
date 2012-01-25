/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0

Rectangle {
    width: 480; height: 640

    Component {
        id: numberDelegate

        Text {
            id: numberText
            anchors { left: parent.left; right: parent.right }
            text: number

            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 18

            Text {
                anchors { left: parent.left; baseline: parent.baseline }
                text: index

                horizontalAlignment: Text.AlignLeft
                font.pixelSize: 12
            }
            Text {
                anchors { right: parent.right; baseline: parent.baseline }
                text: numberText.VisualDataModel.itemsIndex

                horizontalAlignment: Text.AlignRight
                font.pixelSize: 12
            }
        }
    }

    ListView {
        anchors {
            left: parent.left; top: parent.top;
            right: parent.horizontalCenter; bottom: button.top
            leftMargin: 2; topMargin: 2; rightMargin: 1; bottomMargin: 2
        }

        model: ListModel {
            id: unsortedModel
        }
        delegate: numberDelegate
    }
    ListView {
        anchors {
            left: parent.horizontalCenter; top: parent.top;
            right: parent.right; bottom: button.top
            leftMargin: 1; topMargin: 2; rightMargin: 2; bottomMargin: 2
        }
        model: VisualDataModel {
            model: unsortedModel
            delegate: numberDelegate

            items.onChanged: {
                for (var i = 0; i < inserted.length; ++i) {
                    for (var j = inserted[i].index; j < inserted[i].index + inserted[i].count; ++j) {
                        var number = items.get(j).model.number
                        for (var l = 0, k = 0; l < unsortedModel.count; ++l) {
                            if (l == inserted[k].index) {
                                l += inserted[k].count - 1
                                ++k
                            } else if (number < items.get(l).model.number) {
                                items.move(j, l, 1)
                                break
                            }
                        }
                        inserted[i].index += 1;
                        inserted[i].count -= 1;
                    }
                }
            }
        }
    }

    Rectangle {
        id: button

        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 2 }
        height: moreText.implicitHeight + 4

        color: "black"

        Text {
            id: moreText

            anchors.fill: parent
            text: "More"
            color: "white"
            font.pixelSize: 18
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        MouseArea {
            anchors.fill: parent

            onClicked: unsortedModel.append({ "number": Math.floor(Math.random() * 100) })
        }
    }
}
