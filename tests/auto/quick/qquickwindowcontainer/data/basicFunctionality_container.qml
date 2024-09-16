import QtQuick

Window {
    id: topLevel
    Item {
        id: childItem
        x: 100; y: 100

        WindowContainer {
            window: Window {
                objectName: "childWindow"
            }
        }
    }
}
