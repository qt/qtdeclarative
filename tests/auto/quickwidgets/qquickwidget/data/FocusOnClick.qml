import QtQuick

Rectangle {
    width: 300
    height: 100
    color: "lightblue"
    Column {
        Rectangle {
            width: 150
            height: 50
            color: text1.activeFocus ? "pink" : "gray"
            TextInput
            {
                id: text1
                objectName: "text1"
                anchors.fill: parent
                text: "Enter text"
            }
        }
        Rectangle {
            width: 150
            height: 50
            color: text2.activeFocus ? "yellow" : "lightgreen"
            TextInput
            {
                id: text2
                objectName: "text2"
                anchors.fill: parent
                text: "Enter text"
            }
        }
    }
}
