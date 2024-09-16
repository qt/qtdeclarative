import QtQuick

Image {
    id: root
    property var changeSignals: []
    property var statusChanges: []
    property size statusChangedFirstImplicitSize: Qt.size(-1,-1)

    source: "heart.png"

    onFrameCountChanged: root.changeSignals.push("frameCount")
    onCurrentFrameChanged: root.changeSignals.push("currentFrame")
    onSourceSizeChanged: root.changeSignals.push("sourceSize")
    onImplicitWidthChanged: root.changeSignals.push("implicitWidth")
    onImplicitHeightChanged: root.changeSignals.push("implicitHeight")
    onPaintedWidthChanged: root.changeSignals.push("paintedWidth")
    onPaintedHeightChanged: root.changeSignals.push("paintedHeight")
    onProgressChanged: root.changeSignals.push("progress")
    onStatusChanged: (status) => {
        root.changeSignals.push("status")
        root.statusChanges.push(status)
        if (root.statusChangedFirstImplicitSize.width < 0)
            root.statusChangedFirstImplicitSize = Qt.size(root.implicitWidth, root.implicitHeight)
    }
}
