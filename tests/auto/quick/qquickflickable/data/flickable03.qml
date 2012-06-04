import QtQuick 2.0

Flickable {
    width: 100; height: 400
    contentWidth: column.width; contentHeight: column.height

    Column {
        id: column
        Repeater {
            model: 20
            Rectangle { width: 200; height: 300; border.width: 1; color: "yellow" }
        }
    }
}
