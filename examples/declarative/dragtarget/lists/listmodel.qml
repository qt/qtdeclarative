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
