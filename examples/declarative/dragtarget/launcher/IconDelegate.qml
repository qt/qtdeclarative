import QtQuick 2.0

Item {
    id: iconDelegate

    property alias drag: dragArea.drag

    width: 64; height: 64

    Shadow {
        anchors.centerIn: parent
        sourceItem: iconImage
        width: iconImage.width
        height: iconImage.height
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
        }

        transitions: Transition {
            AnchorAnimation { targets: iconImage; duration: 100 }
        }
    }
}
