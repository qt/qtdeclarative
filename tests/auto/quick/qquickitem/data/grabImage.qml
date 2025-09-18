import QtQuick 2.3

Window {
    id: root
    width: 300
    height: 300
    visible: true

    property bool finishedSuccessfuly: false
    property var itemGrabResult: null

    Item {
        id: child
        x: 50
        y: 50
        width: 50
        height: 50

        Component.onCompleted: {
            const grabbed = child.grabToImage(function(itemGrabResult) {
                finishedSuccessfuly = true
                root.itemGrabResult = itemGrabResult
            })
        }
    }
}

