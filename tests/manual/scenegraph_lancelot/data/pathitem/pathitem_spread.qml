import QtQuick 2.9

Item {
    width: 320
    height: 480

    Column {
        Repeater {
            model: 3
            PathItem {
                enableVendorExtensions: false
                width: 200
                height: 150
                VisualPath {
                    strokeColor: "transparent"

                    fillGradient: PathLinearGradient {
                        id: grad
                        y1: 50; y2: 80
                        spread: model.index === 0 ? PathGradient.PadSpread : (model.index === 1 ? PathGradient.RepeatSpread : PathGradient.ReflectSpread)
                        PathGradientStop { position: 0; color: "black" }
                        PathGradientStop { position: 1; color: "red" }
                    }

                    Path {
                        startX: 10; startY: 10
                        PathLine { relativeX: 180; relativeY: 0 }
                        PathLine { relativeX: 0; relativeY: 100 }
                        PathLine { relativeX: -180; relativeY: 0 }
                        PathLine { relativeX: 0; relativeY: -100 }
                    }
                }
            }
        }
    }
}
