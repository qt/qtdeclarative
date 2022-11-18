import QtQuick
import QtQuick.Controls

Rectangle {
    Button {
        text: "Drag me"
        width: 150 // workaround for QTBUG-104954
        DragHandler { }
    }
}
