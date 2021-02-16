import QtQuick
Text {
    font.pixelSize: 2 * subItem.value

    QtObject {
        id: subItem
        property int value: 1
    }
}
