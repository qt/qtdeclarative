import QtQuick

Item {
    id: root
    width: 400
    height: 300

    Loader {
        id: loader
        objectName: "loader"
        sourceComponent: Item {}
    }
    Rectangle {
        id: parentItem
        objectName: "parentItem"
        width: 20
        height: 20
        color: "blue"
    }
    Rectangle {
        id: childItem
        objectName: "childItem"
        parent: parentItem
        width: 10
        height: 10
        color: "red"
    }

    Rectangle {
        id: loaderChild
        objectName: "loaderChild"
        parent: loader.item
        x: 100
        y: 100
        width: 10
        height: 10
        color: "green"
    }
}
