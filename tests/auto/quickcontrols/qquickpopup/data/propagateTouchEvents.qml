import QtQuick
import QtQuick.Controls

Window {
    id: rootItem
    width: 100
    height: 100

    property alias tapHandler: tHandler

    Popup {
        id: popupItem
        width: 50
        height: 50
        background: Item { TapHandler { id: tHandler } }
    }
}
