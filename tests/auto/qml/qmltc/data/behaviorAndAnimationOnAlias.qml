import QtQuick
Rectangle {
    id: root
    width: 100
    property alias widthAlias: root.width
    Behavior on widthAlias {
        NumberAnimation { duration: 1000 }
    }
}
