import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 100
    height: 100

    property alias _drawer: drawer
    property alias _listView: drawer.contentItem
    property alias _dropArea: dropArea

    Drawer {
        id: drawer
        width: 50
        height: parent.height
        contentItem: ListView {
            width: parent.width
            height: contentHeight
            model: 500
            delegate: Rectangle {
                height: 15
                Label {
                    text: modelData
                }
            }
        }
    }

    DropArea {
        id: dropArea
        anchors.fill: parent
        Connections {
            target: drawer
            function onOpened() {
                dropArea.parent = drawer.parent
            }
        }
    }
}
