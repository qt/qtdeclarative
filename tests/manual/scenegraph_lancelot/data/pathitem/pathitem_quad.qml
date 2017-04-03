import QtQuick 2.9
import Qt.labs.pathitem 1.0

Item {
    width: 320
    height: 480

    Column {
        Repeater {
            model: 4
            Item {
                width: 200
                height: 100

                PathItem {
                    anchors.fill: parent
                    enableVendorExtensions: false

                    VisualPath {
                        strokeWidth: (model.index + 2) * 2
                        strokeColor: "black"
                        fillColor: "lightBlue"

                        Path {
                            startX: 50; startY: 100
                            PathQuad {
                                x: 150; y: 100
                                controlX: model.index * 10; controlY: model.index * 5
                            }
                        }
                    }
                }
            }
        }
    }
}
