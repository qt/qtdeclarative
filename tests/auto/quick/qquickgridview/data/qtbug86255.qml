import QtQuick 2.15

Item {
    width: 240
    height: 320

    GridView {
        id: grid
        objectName: "view"
        anchors.fill: parent
        cellWidth: 64
        cellHeight: 64
        model: ListModel {
            id: listModel

            Component.onCompleted: reload()

            function reload() {
                clear();
                for (let i = 0; i < 1000; i++) {
                    let magic = Math.random();
                    append( { magic } );
                }
            }
        }
        clip: true
        delegate: Item {
            id: d
            property string val: magic
            Loader {
                property alias value: d.val
                asynchronous: true
                sourceComponent: cmp
            }
        }
    }

    Timer {
        running: true
        interval: 1000
        onTriggered: listModel.reload()
    }
    Timer {
        running: true
        interval: 500
        onTriggered: grid.flick(0, -4000)
    }

    Component {
        id: cmp
        Text {
            text: value
        }
    }
}
