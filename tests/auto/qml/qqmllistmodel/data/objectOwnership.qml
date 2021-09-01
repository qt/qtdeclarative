import QtQuick

ListView {
    id: root
    width: 100
    height: 100

    delegate: Component {
        Item {
            property Item myItem: refItem
        }
    }

    model: ListModel {
        id: listModel
        objectName: "listModel"

        function addItem() {
            append({"refItem": cppOwnedItem});
        }
    }

    function checkItem() {
        root.currentIndex = 0;
        currentItem.myItem.dummy();
    }
}
