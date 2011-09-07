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


Rectangle {
    id: root
    color: "grey"

    width: 720
    height: 380

    Component {
        id: draggedText
        Text {
            x: rootTarget.dragX - 10
            y: rootTarget.dragY - 10
            width: 20
            height: 20

            text: rootTarget.dragData.display
            font.pixelSize: 18
        }
    }

    DragTarget {
        id: rootTarget

        anchors.fill: parent
    }

    Loader {
        anchors.fill: parent
        sourceComponent: rootTarget.containsDrag ? draggedText : undefined
    }

    GridView {
        id: gridView

        width: 240
        height: 360

        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: 10

        cellWidth: 60
        cellHeight: 60

        model: ListModel {
            id: gridModel

            ListElement { display: "1" }
            ListElement { display: "2" }
            ListElement { display: "3" }
            ListElement { display: "4" }
            ListElement { display: "5" }
            ListElement { display: "6" }
            ListElement { display: "7" }
            ListElement { display: "8" }
            ListElement { display: "9" }
            ListElement { display: "10" }
            ListElement { display: "11" }
            ListElement { display: "12" }
            ListElement { display: "13" }
            ListElement { display: "14" }
            ListElement { display: "15" }
            ListElement { display: "16" }
            ListElement { display: "17" }
            ListElement { display: "18" }
            ListElement { display: "19" }
            ListElement { display: "20" }
            ListElement { display: "21" }
            ListElement { display: "22" }
            ListElement { display: "23" }
            ListElement { display: "24" }
        }

        delegate: Rectangle {
            id: root

            width: 60
            height: 60

            color: "black"

            Text {
                anchors.fill: parent
                color: draggable.drag.active ? "gold" : "white"
                text: display
                font.pixelSize: 16
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }

            MouseArea {
                id: draggable

                property int initialIndex

                width: 60
                height: 60

                drag.data: model
                drag.keys: ["grid"]
                drag.target: draggable

                states: State {
                    when: !draggable.drag.active
                    PropertyChanges { target: draggable; x: 0; y: 0 }
                }
            }
        }

        DragTarget {
            anchors.fill: parent

            keys: [ "grid" ]
            onPositionChanged: {
                var index = gridView.indexAt(drag.x, drag.y)
                if (index != -1)
                    gridModel.move(drag.data.index, index, 1)
            }
        }

        DragTarget {
            property int dragIndex
            anchors.fill: parent

            keys: [ "list" ]
            onEntered: {
                dragIndex = gridView.indexAt(drag.x, drag.y)
                if (dragIndex != -1) {
                    gridModel.insert(dragIndex, { "display": drag.data.display })
                } else {
                    event.accepted = false
                }
            }
            onPositionChanged: {
                var index = gridView.indexAt(drag.x, drag.y);
                if (index != -1) {
                    gridModel.move(dragIndex, index, 1)
                    dragIndex = index
                }
            }
            onExited: gridModel.remove(dragIndex, 1)
        }
    }

    ListView {
        id: listView

        width: 240
        height: 360

        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: 10

        model: ListModel {
            id: listModel

            ListElement { display: "a" }
            ListElement { display: "b" }
            ListElement { display: "c" }
            ListElement { display: "d"}
            ListElement { display: "e" }
            ListElement { display: "f" }
            ListElement { display: "g" }
            ListElement { display: "h" }
            ListElement { display: "i" }
            ListElement { display: "j" }
            ListElement { display: "k" }
            ListElement { display: "l" }
            ListElement { display: "m" }
            ListElement { display: "n" }
            ListElement { display: "o" }
            ListElement { display: "p" }
            ListElement { display: "q" }
            ListElement { display: "r" }
            ListElement { display: "s" }
            ListElement { display: "t" }
            ListElement { display: "u" }
            ListElement { display: "v" }
            ListElement { display: "w" }
            ListElement { display: "x" }
        }

        delegate: Rectangle {
            id: root

            width: 240
            height: 15

            color: "black"

            Text {
                anchors.fill: parent
                color: draggable.drag.active ? "gold" : "white"
                text: display
                font.pixelSize: 12
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }

            MouseArea {
                id: draggable

                width: 240
                height: 15

                drag.data: model
                drag.keys: ["list"]
                drag.target: draggable

                states: State {
                    when: !draggable.drag.active
                    PropertyChanges { target: draggable; x: 0; y: 0 }
                }
            }
        }

        DragTarget {
            anchors.fill: parent

            keys: [ "list" ]
            onPositionChanged: {
                var index = listView.indexAt(drag.x, drag.y)
                if (index != -1)
                    listModel.move(drag.data.index, index, 1)
            }
        }

        DragTarget {
            property int dragIndex
            anchors.fill: parent

            keys: [ "grid" ]

            onEntered: {
                dragIndex = listView.indexAt(drag.x, drag.y)
                if (dragIndex != -1) {
                    listModel.insert(dragIndex, { "display": drag.data.display })
                } else {
                    event.accepted = false
                }
            }
            onPositionChanged: {
                var index = listView.indexAt(drag.x, drag.y);
                if (index != -1) {
                    listModel.move(dragIndex, index, 1)
                    dragIndex = index
                }
            }
            onExited: listModel.remove(dragIndex, 1)
        }
    }
}
