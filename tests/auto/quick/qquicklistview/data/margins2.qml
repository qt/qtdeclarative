import QtQuick 2.0

Item {
    width: 200; height: 200
    Page {
        Rectangle {
            anchors.fill: parent
            color: "lightsteelblue"
        }
        ListView {
            objectName: "listview"
            topMargin: 20
            bottomMargin: 20
            leftMargin: 20
            rightMargin: 20
            anchors.fill: parent

            model: 20
            delegate: Rectangle {
                color: "skyblue"
                width: 60; height: 60
                Text {
                    id: txt
                    text: "test" + index
                }
            }
        }
    }
}
