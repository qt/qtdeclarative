import QtQuick
import QtQuick.Shapes

Shape {
    objectName: "shape"

    ShapePath {
        objectName: "small"
        strokeColor: "transparent"
        PathRectangle {
            width: 10
            height: 10
        }
    }

    ShapePath {
        objectName: "large"
        strokeColor: "transparent"
        PathRectangle {
            width: 100
            height: 80
        }
    }

    ShapePath {
        objectName: "distant"
        visible: false
        strokeColor: "transparent"
        PathRectangle {
            x: 400
            y: 300
            width: 20
            height: 20
        }
    }
}
