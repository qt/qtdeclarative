import QtQuick 2.9

Item {
    width: 200
    height: 150

    PathItem {
        enableVendorExtensions: false
        objectName: "pathItem"
        anchors.fill: parent

        VisualPath {
            strokeColor: "red"
            fillColor: "green"
            Path {
                startX: 40; startY: 30
                PathQuad { x: 50; y: 80; controlX: 0; controlY: 80 }
                PathLine { x: 150; y: 80 }
                PathQuad { x: 160; y: 30; controlX: 200; controlY: 80 }
            }
        }

        VisualPath {
            strokeWidth: 10
            fillColor: "transparent"
            strokeColor: "blue"
            Path {
                startX: 40; startY: 30
                PathCubic { x: 50; y: 80; control1X: 0; control1Y: 80; control2X: 100; control2Y: 100 }
            }
        }

        VisualPath {
            fillGradient: PathLinearGradient {
                y2: 150
                PathGradientStop { position: 0; color: "yellow" }
                PathGradientStop { position: 1; color: "green" }
            }

            Path {
                startX: 10; startY: 100
                PathArc {
                    relativeX: 50; y: 100
                    radiusX: 25; radiusY: 25
                }
                PathArc {
                    relativeX: 50; y: 100
                    radiusX: 25; radiusY: 35
                }
                PathArc {
                    relativeX: 50; y: 100
                    radiusX: 25; radiusY: 60
                }
                PathArc {
                    relativeX: 50; y: 100
                    radiusX: 50; radiusY: 120
                }
            }
        }
    }
}
