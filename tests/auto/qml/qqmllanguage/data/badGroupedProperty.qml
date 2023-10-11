import QtQml 2.15

QtObject {
    id: testItem
    property rect rect
    onComplete {
        rect.x: 2
        rect.width: 22
    }
}
