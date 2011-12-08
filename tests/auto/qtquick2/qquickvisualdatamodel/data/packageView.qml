import QtQuick 2.0

Item {
    width: 240
    height: 320

    Component {
        id: myDelegate

        Package {
            Rectangle {
                id: leftWrapper
                objectName: "wrapper"
                Package.name: "left"
                height: 20
                width: 120
                Text {
                    text: index
                }
                color: ListView.isCurrentItem ? "lightsteelblue" : "white"
            }
            Rectangle {
                id: rightWrapper
                objectName: "wrapper"
                Package.name: "right"
                height: 20
                width: 120
                Text {
                    text: index
                }
                color: ListView.isCurrentItem ? "lightsteelblue" : "white"
            }
        }

    }

    VisualDataModel {
        id: visualModel

        delegate: myDelegate
        model: testModel
    }

    ListView {
        id: leftList
        objectName: "leftList"
        anchors {
            left: parent.left; top: parent.top;
            right: parent.horizontalCenter; bottom: parent.bottom
        }
        model: visualModel.parts.left
        currentIndex: 0
    }

    ListView {
        id: rightList
        objectName: "rightList"
        anchors {
            left: parent.horizontalCenter; top: parent.top;
            right: parent.right; bottom: parent.bottom
        }
        model: visualModel.parts.right
        currentIndex: 20
    }
}
