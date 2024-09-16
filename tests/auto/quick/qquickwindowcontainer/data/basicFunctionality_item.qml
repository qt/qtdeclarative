import QtQuick

Window {
    id: topLevel
    Item {
        id: childItem
        x: 100; y: 100

        Window {
            parent: childItem
            objectName: "childWindow"
            x: 100; y: 100
        }
    }
}
