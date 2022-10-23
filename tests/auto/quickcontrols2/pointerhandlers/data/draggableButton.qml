import QtQuick
import QtQuick.Controls

Rectangle {
    width: 150; height: 150
    Button {
        text: "Drag me"
        width: 150 // workaround for QTBUG-104954
        DragHandler { }
    }
}
