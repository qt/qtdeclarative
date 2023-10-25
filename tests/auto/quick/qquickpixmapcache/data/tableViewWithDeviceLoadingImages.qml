import QtQuick
import PixmapCacheTest

TableView {
    id: root
    width: 640
    height: 480
    model: 100
    rowSpacing: 6
    property real size: 40
    columnWidthProvider: function(col) { return root.size }
    rowHeightProvider: function(col) { return root.size }

    Timer {
        interval: 200
        repeat: true
        running: true
        onTriggered: {
            root.size = Math.random() * 200
            root.positionViewAtRow(Math.round(Math.random() * 100), TableView.Visible)
        }
    }

    delegate: DeviceLoadingImage {
        required property int index
        width: root.size; height: root.size
        asynchronous: true
        source: "image://slow/" + index
        sourceSize.width: width
        sourceSize.height: height

        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.color: "red"
        }

        Text {
            color: "red"
            style: Text.Outline
            styleColor: "white"
            text: index + "\nsize " + root.size.toFixed(1)
        }
    }
}
