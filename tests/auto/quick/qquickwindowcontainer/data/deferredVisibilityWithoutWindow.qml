import QtQuick

Window {
    id: topLevel
    visible: true
    Window {
        parent: topLevel
        objectName: "childWindow"
        visible: true
    }
}
