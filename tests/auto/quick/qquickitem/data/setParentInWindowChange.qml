import QtQuick 2.12

Rectangle {
    width: 800
    height: 600
    Item {
        id: it
        onWindowChanged: () => it.parent = newParent
    }

    Item { id: newParent }
}
