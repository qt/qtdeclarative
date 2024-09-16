import QtQuick

Window {
    id: topLevel
    Window {
        parent: topLevel
        objectName: "childWindow"
        x: 100; y: 100
    }
}
