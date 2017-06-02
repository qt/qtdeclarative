import QtQuick 2.9
import tst_qquickpathitem 1.0

Item {
    width: 200
    height: 150

    Shape {
        enableVendorExtensions: false
        objectName: "pathItem"
        anchors.fill: parent

        ShapePath {
            strokeWidth: 4
            strokeColor: "red"
            fillGradient: ShapeLinearGradient {
                x1: 20; y1: 20
                x2: 180; y2: 130
                ShapeGradientStop { position: 0; color: "blue" }
                ShapeGradientStop { position: 0.2; color: "green" }
                ShapeGradientStop { position: 0.4; color: "red" }
                ShapeGradientStop { position: 0.6; color: "yellow" }
                ShapeGradientStop { position: 1; color: "cyan" }
            }
            strokeStyle: ShapePath.DashLine
            dashPattern: [ 1, 4 ]
            startX: 20; startY: 20
            PathLine { x: 180; y: 130 }
            PathLine { x: 20; y: 130 }
            PathLine { x: 20; y: 20 }
        }
    }
}
