import QtQuick 2.0

PathView {
    id: view
    property int delegatesCreated: 0
    property int delegatesDestroyed: 0

    width: 400
    height: 400
    preferredHighlightBegin: 0.5
    preferredHighlightEnd: 0.5
    pathItemCount: 5
    currentIndex: 1
    model: customModel
    delegate: Text {
        text: "item: " + index + " of: " + view.count
        Component.onCompleted: view.delegatesCreated++;
        Component.onDestruction: view.delegatesDestroyed++;
    }
    path: Path {
        startX: 50
        startY: 0
        PathLine {
            x: 50
            y: 400
        }
    }
}
