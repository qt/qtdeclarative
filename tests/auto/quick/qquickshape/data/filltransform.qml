import QtQuick
import QtQuick.Shapes
import tst_qquickpathitem

Rectangle {
    width: 440
    height: 220
    color: "white"

    Shape {
        objectName: "shape1"
        ShapePath {
            id: path1
            objectName: "path1"
            fillGradient: RadialGradient {
                centerX: path1.startX + 100
                centerY: path1.startY + 100
                centerRadius: 100
                focalX: centerX
                focalY: centerY
                GradientStop { position: 0.0; color: "blue" }
                GradientStop { position: 0.5; color: "cyan" }
                GradientStop { position: 1.0; color: "blue" }
            }

            fillTransform: PlanarTransform.fromScale(2, 1);

            startX: 10
            startY: 10
            PathLine { relativeX: 200; relativeY: 0 }
            PathLine { relativeX: 0; relativeY: 200 }
            PathLine { relativeX: -200; relativeY: 0 }
            PathLine { relativeX: 0; relativeY: -200 }
        }

        ShapePath {
            id: path2
            objectName: "path2"
            fillGradient: RadialGradient {
                centerX: path2.startX + 100
                centerY: path2.startY + 100
                centerRadius: 100
                focalX: centerX
                focalY: centerY
                GradientStop { position: 0.0; color: "blue" }
                GradientStop { position: 0.5; color: "cyan" }
                GradientStop { position: 1.0; color: "blue" }
            }

            startX: 220 + 10
            startY: 10
            PathLine { relativeX: 200; relativeY: 0 }
            PathLine { relativeX: 0; relativeY: 200 }
            PathLine { relativeX: -200; relativeY: 0 }
            PathLine { relativeX: 0; relativeY: -200 }
        }
    }
}
