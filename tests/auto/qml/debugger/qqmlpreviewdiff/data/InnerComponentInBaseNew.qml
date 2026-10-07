import QtQuick

Holder {
    component Holder: Item {
        id: holder
        property int base: 1
        Item {
            id: outerChild
            objectName: "outerChild"
            property int value: holder.base + 1
        }
        property Component comp: Component {
            Item {
                property int value: outerChild.value + 10
                Item { objectName: "added" }
            }
        }
    }
}
