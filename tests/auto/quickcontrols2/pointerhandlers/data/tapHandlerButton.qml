import QtQuick
import QtQuick.Controls

Rectangle {
    width: 150; height: 150
    color: th.pressed ? "lightsteelblue" : "beige"
    Button {
        text: pressed ? "pressed" : ""
        width: 150 // workaround for QTBUG-104954
        TapHandler { id: th }
    }
}
