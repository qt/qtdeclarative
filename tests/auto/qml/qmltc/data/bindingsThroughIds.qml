import QtQuick
Item {
    id: theItem
    property string text: theText.text
    width: theRect.width

    Text {
        id: theText
        text: "theText.text"
        color: "red"
    }

    Rectangle {
        id: theRect
        color: theText.color
        width: 42
    }
}
