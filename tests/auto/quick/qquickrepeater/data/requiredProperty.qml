import QtQuick 2.14

Item {
    Column {
        Repeater {
            model: ["apples", "oranges", "pears"]
            Text {
                id: txt
                required property string modelData
                required property int index
                text: modelData + index
                Component.onCompleted: () => {console.info(txt.text)}
            }
        }
    }
}
