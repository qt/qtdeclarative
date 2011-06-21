import QtQuick 2.0

Item {
    id: iconDelegate

    property alias drag: dragArea.drag

    width: 64; height: 64

    Rectangle {
        anchors.fill: parent
        opacity: dragArea.drag.active ? 0.5 : 0.0
        color: "lightgrey"
        radius: 12

        Behavior on opacity { NumberAnimation { duration: 300 } }
    }

    MouseArea {
        id: dragArea

        property real rootX
        property real rootY

        width: iconImage.implicitWidth; height:  iconImage.implicitHeight
        anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter }

        drag.target: dragArea
        drag.data: index

        Binding {
            target: dragArea
            property: "rootX"
            value: dragArea.mapToItem(root, x, y).x
            when: !dragArea.drag.active
        }
        Binding {
            target: dragArea
            property: "rootY"
            value: dragArea.mapToItem(root, x, y).y
            when: !dragArea.drag.active
        }

        Image {
            id: iconImage

            anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter }

            source: icon
        }

        states: State {
            when: dragArea.drag.active

            AnchorChanges {
                target: dragArea;
                anchors { horizontalCenter: undefined; verticalCenter: undefined }
            }
            ParentChange { target: dragArea; parent: root; x: dragArea.rootX; y:dragArea.rootY }
            AnchorChanges {
                target: iconImage;
                anchors { verticalCenter: undefined; bottom: dragArea.verticalCenter }
            }
        }

        transitions: Transition {
            AnchorAnimation { targets: iconImage; duration: 100 }
        }
    }
}
