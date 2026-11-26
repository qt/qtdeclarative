import QtQuick
import InventoryTypes

ListView {
    id: listView
    anchors.fill: parent
    model: InventoryModel {}
    delegate: Text {
        width: listView.width
        text: itemName

        required property int index
        required property string itemName
        required property ComponentEntity entity
    }

    function removeLast() { model.removeLast() }
}


