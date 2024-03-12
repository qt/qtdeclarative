// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick

Rectangle {
    id: editor
    color: "lightGrey"
    width: 640
    height: 480

    Rectangle {
        color: "white"
        anchors.fill: parent
        anchors.margins: 20

        BorderImage {
            id: startHandle
            source: "pics/startHandle.sci"
            opacity: 0.0
            width: 10
            x: edit.positionToRectangle(edit.selectionStart).x - flick.contentX-width
            y: edit.positionToRectangle(edit.selectionStart).y - flick.contentY
            height: edit.positionToRectangle(edit.selectionStart).height
        }

        BorderImage {
            id: endHandle
            source: "pics/endHandle.sci"
            opacity: 0.0
            width: 10
            x: edit.positionToRectangle(edit.selectionEnd).x - flick.contentX
            y: edit.positionToRectangle(edit.selectionEnd).y - flick.contentY
            height: edit.positionToRectangle(edit.selectionEnd).height
        }

        Flickable {
            id: flick

            anchors.fill: parent
            contentWidth: edit.contentWidth
            contentHeight: edit.contentHeight
            interactive: true
            clip: true

            function ensureVisible(r) {
                if (contentX >= r.x)
                    contentX = r.x;
                else if (contentX+width <= r.x+r.width)
                    contentX = r.x+r.width-width;
                if (contentY >= r.y)
                    contentY = r.y;
                else if (contentY+height <= r.y+r.height)
                    contentY = r.y+r.height-height;
            }

            TextEdit {
                id: edit
                width: flick.width
                height: flick.height
                focus: true
                wrapMode: TextEdit.Wrap
                textFormat: TextEdit.RichText

                onCursorRectangleChanged: flick.ensureVisible(cursorRectangle)

                text: "<h1>Text Selection</h1>"
                    +"<p>This example is a whacky text selection mechanisms, showing how these can be implemented in the TextEdit element, to cater for whatever style is appropriate for the target platform."
                    +"<p><b>Press-and-hold</b> to select a word, then drag the selection handles."
                    +"<p><b>Drag outside the selection</b> to scroll the text."
                    +"<p><b>Click inside the selection</b> to cut/copy/paste/cancel selection."
                    +"<p>It's too whacky to let you paste if there is no current selection."

            }
        }

        Item {
            id: menu
            opacity: 0.0
            width: 100
            height: 120
            anchors.centerIn: parent

            Rectangle {
                border.width: 1
                border.color: "darkBlue"
                radius: 15
                color: "#806080FF"
                anchors.fill: parent
            }

            Column {
                anchors.centerIn: parent
                spacing: 8

                Rectangle {
                    border.width: 1
                    border.color: "darkBlue"
                    color: "#ff7090FF"
                    width: 60
                    height: 16

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("Cut")
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: function() {
                            edit.cut()
                            editor.state = ""
                        }
                    }
                }

                Rectangle {
                    border.width: 1
                    border.color: "darkBlue"
                    color: "#ff7090FF"
                    width: 60
                    height: 16

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("Copy")
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: function() {
                            edit.copy()
                            editor.state = "selection"
                        }
                    }
                }

                Rectangle {
                    border.width: 1
                    border.color: "darkBlue"
                    color: "#ff7090FF"
                    width: 60
                    height: 16

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("Paste")
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: function() {
                            edit.paste()
                            edit.cursorPosition = edit.selectionEnd
                            editor.state = ""
                        }
                    }
                }

                Rectangle {
                    border.width: 1
                    border.color: "darkBlue"
                    color: "#ff7090FF"
                    width: 60
                    height: 16

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("Deselect")
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            edit.cursorPosition = edit.selectionEnd;
                            edit.deselect();
                            editor.state = ""
                        }
                    }
                }
            }
        }
    }

    states: [
        State {
            name: "selection"
            PropertyChanges {
                startHandle.opacity: 1.0
                endHandle.opacity: 1.0
            }
        },
        State {
            name: "menu"
            PropertyChanges {
                startHandle.opacity: 0.5
                endHandle.opacity: 0.5
                menu.opacity: 1.0
            }
        }
    ]
}
