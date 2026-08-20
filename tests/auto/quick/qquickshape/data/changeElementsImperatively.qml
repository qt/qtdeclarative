import QtQuick
import QtQuick.Shapes

Shape {
    width: 640
    height: 480

    Component.onCompleted: {
        bottomPathLine.x = 20
    }

    ShapePath {
        PathLine {}
        PathLine {
            id: bottomPathLine
        }
        PathArc {}
        PathLine {}
        PathArc {}
    }
}
