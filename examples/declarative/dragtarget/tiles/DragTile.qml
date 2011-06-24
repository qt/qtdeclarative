import QtQuick 2.0

Rectangle {
    id: dragRectangle

    property Item dropTarget

    property string colorKey

    color: colorKey

    width: 100; height: 100

    Text {
        anchors.fill: parent
        color: "white"
        font.pixelSize: 90
        text: modelData + 1
        horizontalAlignment:Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    MouseArea {
        id: draggable

        anchors.fill: parent

        drag.target: parent
        drag.keys: [ colorKey ]

        drag.onDropped: dropTarget = dropItem

        states: [
            State {
                when: dragRectangle.dropTarget != undefined && !draggable.drag.active
                ParentChange {
                    target: dragRectangle
                    parent: dropTarget
                    x: 0
                    y: 0
                }
            },
            State {
                when: dragRectangle.dropTarget != undefined && draggable.drag.active
                ParentChange {
                    target: dragRectangle
                    parent: dropTarget
                }
            },
            State {
                when:  !draggable.drag.active
                AnchorChanges {
                    target: dragRectangle
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        ]
    }
}
