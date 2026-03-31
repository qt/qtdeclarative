import QtQuick

Item {
    Item { id: child }
    component IC: Item {
        Item {
            id: childInIC
        }
    }
}
