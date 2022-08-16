import QtQuick 2.12

Item {
    width: 240; height: 240
    Loader {
        id: loader
        sourceComponent: surfaceParent
        anchors.fill: parent

        onStatusChanged: {
            if (status === Loader.Ready) {
                holder.create()
                holder.item.parent = item
            } else if (status === Loader.Null){
                holder.item.parent = null
            }
        }
    }

    property var holder: QtObject {
        property bool created: false
        function create()
        {
            if (!created)
                surfaceComponent.createObject(item)
            created = true
        }

        property Item item: Item {
            anchors.fill: parent
            Component {
                id: surfaceComponent
                Item {
                    anchors.fill: parent
                    TextInput {
                        width: parent.width
                        font.pixelSize: 40
                        text: "focus me"
                    }
                }
            }
        }
    }

    Component {
        id: surfaceParent
        Rectangle {
            anchors.fill: parent
        }
    }
}
