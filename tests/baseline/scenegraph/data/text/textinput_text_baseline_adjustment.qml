import QtQuick 2.0

Item {
    width: 400
    height: 400

    ListModel {
        id: model
        ListElement { name: "20px, bottom"; vAlign: Text.AlignBottom; size: 20 }
        ListElement { name: "20px, top"; vAlign: Text.AlignTop; size: 20 }
        ListElement { name: "20px, center"; vAlign: Text.AlignVCenter; size: 20 }
        ListElement { name: "30px, bottom"; vAlign: Text.AlignBottom; size: 30 }
        ListElement { name: "30px, top"; vAlign: Text.AlignTop; size: 30 }
        ListElement { name: "30px, center"; vAlign: Text.AlignVCenter; size: 30 }
    }

    Column {
        anchors.fill: parent
        spacing: 20
        Repeater {
            anchors.fill: parent
            model: model
            Row {
                height: 40
                Text {
                    text: name + ": "
                    width: 90
                    verticalAlignment: Text.AlignBottom
                }

                TextInput {
                    text: "foo😀"
                    verticalAlignment: vAlign
                    font.pixelSize: size
                    font.family: "Consolas"

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 1
                        opacity: 0.25
                        color: "blue"
                        anchors.bottom: parent.baseline
                    }
                }

                Text {
                    text: "foo😀"
                    verticalAlignment: vAlign
                    font.pixelSize: size
                    font.family: "Consolas"

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 1
                        opacity: 0.25
                        color: "blue"
                        anchors.bottom: parent.baseline
                    }

                }

                TextEdit {
                    textFormat: TextEdit.PlainText
                    text: "foo😀"
                    verticalAlignment: vAlign
                    font.pixelSize: size
                    font.family: "Consolas"

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 1
                        opacity: 0.25
                        color: "blue"
                        anchors.bottom: parent.baseline
                    }
                }
            }
        }
    }

}
