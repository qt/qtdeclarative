import QtQuick
import QtQuick.Controls

Rectangle {
    color: th.pressed ? "lightsteelblue" : "beige"
    Button {
        text: pressed ? "pressed" : ""
        width: 150 // workaround for QTBUG-104954
        TapHandler { id: th }
    }
}
