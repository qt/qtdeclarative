import QtQuick 2.9
import QtQuick.Shapes 1.0

Item {
    width: 320
    height: 480

    PathItem {
        enableVendorExtensions: false

        anchors.fill: parent

        VisualPath {
            strokeWidth: 1
            strokeColor: "red"
            fillColor: "transparent"
            Path {
                PathLine { x: 50; y: 50 }
            }
        }
        VisualPath {
            strokeWidth: 2
            strokeColor: "blue"
            fillColor: "transparent"
            Path {
                startX: 20
                PathLine { x: 70; y: 50 }
            }
        }
        VisualPath {
            strokeWidth: 3
            strokeColor: "green"
            fillColor: "transparent"
            Path {
                startX: 40
                PathLine { x: 90; y: 50 }
            }
        }
        VisualPath {
            strokeWidth: 4
            strokeColor: "yellow"
            fillColor: "transparent"
            Path {
                startX: 60
                PathLine { x: 110; y: 50 }
            }
        }
        VisualPath {
            strokeWidth: 5
            strokeColor: "black"
            fillColor: "transparent"
            strokeStyle: VisualPath.DashLine
            Path {
                startX: 80
                PathLine { x: 130; y: 50 }
            }
        }

        VisualPath {
            strokeWidth: 20
            strokeColor: "gray"
            fillColor: "transparent"
            capStyle: VisualPath.RoundCap
            Path {
                startX: 120; startY: 20
                PathLine { x: 200; y: 100 }
            }
        }

        VisualPath {
            strokeColor: "black"
            strokeWidth: 16
            fillColor: "transparent"
            capStyle: VisualPath.RoundCap
            joinStyle: VisualPath.BevelJoin
            Path {
                startX: 20
                startY: 100
                PathLine { x: 120; y: 200 }
                PathLine { x: 50; y: 200 }
            }
        }
        VisualPath {
            strokeColor: "black"
            strokeWidth: 16
            fillColor: "transparent"
            capStyle: VisualPath.RoundCap
            joinStyle: VisualPath.MiterJoin
            Path {
                startX: 150
                startY: 100
                PathLine { x: 250; y: 200 }
                PathLine { x: 180; y: 200 }
            }
        }
        VisualPath {
            strokeColor: "black"
            strokeWidth: 16
            fillColor: "transparent"
            capStyle: VisualPath.RoundCap
            joinStyle: VisualPath.RoundJoin
            Path {
                startX: 270
                startY: 100
                PathLine { x: 310; y: 200 }
                PathLine { x: 280; y: 200 }
            }
        }
    }
}
