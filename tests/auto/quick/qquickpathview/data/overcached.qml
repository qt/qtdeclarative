import QtQuick 2.0

PathView {
    id: view
    property int delegatesCreated: 0
    property int delegatesDestroyed: 0

    anchors.fill: parent
    preferredHighlightBegin: 0.5
    preferredHighlightEnd: 0.5
    pathItemCount: 10
    interactive: true
    model: 15
    // Much bigger cache than model - pathItemCount
    cacheItemCount: 100
    delegate: Text {
        text: modelData
        Component.onCompleted: view.delegatesCreated++;
        Component.onDestruction: view.delegatesDestroyed++;
    }
    path: Path {
        PathLine {
            x: 0
            y: 400
        }
    }
}
