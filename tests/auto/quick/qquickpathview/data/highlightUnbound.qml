import QtQuick

PathView {
    width: 100
    height: 100
    model: 3
    delegate: Item { width: 10; height: 10 }
    highlight: Rectangle { width: 10; height: 10 }
    path: Path {
        startX: 0
        startY: 0
        PathLine { x: 100; y: 100 }
    }
}
