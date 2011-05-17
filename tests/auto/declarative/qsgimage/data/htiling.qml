import QtQuick 2.0

Rectangle {
    width: 200; height: 550

    Image {
        objectName: "tiling"; anchors.fill: parent
        source: "green.png"; fillMode: Image.TileHorizontally
    }
}

