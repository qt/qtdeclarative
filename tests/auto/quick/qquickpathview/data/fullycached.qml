import QtQuick

// model and cacheItemCount are supplied as initial properties, such that
// pathItemCount + cacheItemCount == model and the whole model gets created.
PathView {
    id: view

    width: 456
    height: 520

    pathItemCount: 3
    preferredHighlightBegin: 0.5
    preferredHighlightEnd: 0.5
    highlightRangeMode: PathView.StrictlyEnforceRange

    required model
    required cacheItemCount

    delegate: Rectangle {
        objectName: "wrapper"
        width: view.width
        height: view.height
        color: "yellow"
        border.color: "green"
    }

    path: Path {
        startX: view.width / 2
        startY: -view.height
        PathLine { x: view.width / 2; y: view.height * 2 }
    }
}
