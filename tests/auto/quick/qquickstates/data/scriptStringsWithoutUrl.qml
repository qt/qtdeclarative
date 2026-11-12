import QtQuick

Rectangle {
    width: 400; height: 400
    Item {
        id: newParent
        objectName: "NewParent"
    }
    Rectangle {
        id: myRect
        objectName: "MyRect"
        width: 50; height: 50
    }
    states: State {
        name: "reparented"
        ParentChange {
            target: myRect
            parent: newParent
            width: 10 + 20
            height: 20 + 20
        }
    }
}
