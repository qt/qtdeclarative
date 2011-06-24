import QtQuick 2.0

Item {
    id: root
    width: 320; height: 480

    Rectangle {
        id: inputRect
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        anchors.margins: 2
        height: input.implicitHeight + 4

        border.width: 1

        TextInput {
            id: input
            anchors.fill: parent; anchors.margins: 2

            text: "the quick brown fox jumped over the lazy dog"

            DragTarget {
                id: inputTarget

                anchors.fill: parent

                Component {
                    id: draggedInputText
                    Text {
                        x: inputTarget.dragX
                        y: inputTarget.dragY
                        text: inputTarget.dragData
                        color: "blue"
                        font: input.font
                    }
                }

                Loader {
                    sourceComponent: parent.containsDrag ? draggedInputText : undefined
                }
            }


            MouseArea {
                id: inputDraggable

                anchors.fill: parent
                enabled: input.selectionStart != input.selectionEnd

                drag.data: input.selectedText
                drag.target: inputDraggable

                drag.onDragged: {
                    var position = input.positionAt(mouse.x);
                    mouse.accepted = position >= input.selectionStart && position < input.selectionEnd
                }

                MouseArea {
                    anchors.fill: parent

                    onPressed: {
                        var position = input.positionAt(mouse.x);
                        if (position < input.selectionStart || position >= input.selectionEnd) {
                            input.cursorPosition = position
                        } else {
                            mouse.accepted = false
                        }
                    }
                    onPositionChanged: input.moveCursorSelection(input.positionAt(mouse.x))
                }
            }
        }
    }

    Rectangle {
        id: editRect
        anchors.left: parent.left; anchors.right: parent.right;
        anchors.top: inputRect.bottom; anchors.bottom: parent.bottom
        anchors.margins: 2

        border.width: 1

        TextEdit {
            id: edit
            anchors.fill: parent; anchors.margins: 2

            text: "the quick brown fox jumped over the lazy dog"
            font.pixelSize: 18
            wrapMode: TextEdit.WordWrap

            DragTarget {
                id: editTarget

                anchors.fill: parent


                Component {
                    id: draggedEditText
                    Text {
                        x: editTarget.dragX
                        y: editTarget.dragY
                        text: editTarget.dragData
                        color: "red"
                        font: edit.font
                    }
                }

                Loader {
                    sourceComponent: parent.containsDrag ? draggedEditText : undefined
                }
            }

            MouseArea {
                id: editDraggable

                anchors.fill: parent
                enabled: edit.selectionStart != edit.selectionEnd

                drag.data: edit.selectedText
                drag.target: editDraggable

                drag.onDragged: {
                    var position = edit.positionAt(mouse.x, mouse.y);
                    mouse.accepted = position >= edit.selectionStart && position < edit.selectionEnd
                }

                MouseArea {
                    anchors.fill: parent

                    onPressed: {
                        var position = edit.positionAt(mouse.x, mouse.y);
                        if (position < edit.selectionStart || position >= edit.selectionEnd) {
                            edit.cursorPosition = position
                        } else {
                            mouse.accepted = false
                        }
                    }
                    onPositionChanged: edit.moveCursorSelection(edit.positionAt(mouse.x, mouse.y))
                }
            }
        }
    }
}
