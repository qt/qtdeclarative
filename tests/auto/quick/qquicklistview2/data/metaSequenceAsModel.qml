import QtQuick
import Test

ListViewWithSequences {
    id: view
    rects: [ Qt.rect(1, 2, 3, 4), Qt.rect(5, 6, 7, 8) ]

    model: rects
    delegate: Item {
        Component.onCompleted: view.texts.push(modelData.x + "/" + modelData.y)
    }
}
