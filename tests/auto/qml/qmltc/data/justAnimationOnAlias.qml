import QtQuick
Rectangle {
    id: root
    width: 100
    property alias widthAlias: root.width
    NumberAnimation on widthAlias { duration: 1000 }
}
