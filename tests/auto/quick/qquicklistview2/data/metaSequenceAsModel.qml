import QtQuick

ListView {
    id: view
    width: 100
    height: 100
    property list<rect> rects: [ Qt.rect(1, 2, 3, 4), Qt.rect(5, 6, 7, 8) ]
    property list<string> texts

    model: rects
    delegate: Item {
        Component.onCompleted: view.texts.push(modelData.x + "/" + modelData.y)
    }
}
