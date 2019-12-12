import QtQuick 2.14

Item {
    Column {
        Repeater {
            model: ["apples", "oranges", "pears"]
            Text {
                id: txt
                text: modelData + index
            }
        }
    }
}
