import QtQuick

Flickable {
    width: 300
    height: 300
    contentWidth: content.width; contentHeight: content.height
    Rectangle {
        id: content
        width: 350
        height: 350
        color: "darkkhaki"
    }
    boundsBehavior: Flickable.StopAtBounds
}
