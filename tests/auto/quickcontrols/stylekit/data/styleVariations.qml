import QtQuick
import Qt.labs.StyleKit

Item {
    width: 640
    height: 480

    property alias insideFrame: insideFrame
    property alias outsideFrame: outsideFrame
    property alias insideFrame_instanceVariation: insideFrame_instanceVariation
    property alias outsideFrame_instanceVariation: outsideFrame_instanceVariation

    StyleKit.style: Style {
        // Pin to Light theme so the test results are consistent regardless of the OS theme
        themeName: "Light"

        StyleVariation {
            id: typeVariation
            button.background.border.width: 4
        }

        StyleVariation {
            name: "instanceVariation"
            button.background.border.width: 10
            button.background.radius: 8
        }

        button {
            background.border.width: 2
            background.radius: 4
        }

        frame {
            variations: [typeVariation]
        }

        light: Theme {
            StyleVariation {
                name: "instanceVariation"
                button.background.border.width: 5
            }

            frame {
                variations: [typeVariation]
            }

            button {
                background.border.width: 3
            }
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 50
        spacing: 10

        Button {
            id: outsideFrame
            text: "outsideFrame"
        }

        Button {
            id: outsideFrame_instanceVariation
            text: "outsideFrame_instanceVariation"
            StyleVariation.variations: ["instanceVariation"]
        }

        Frame {
            Column {
                spacing: 10
                anchors.fill: parent
                Button {
                    id: insideFrame
                    text: "insideFrame"
                }

                Button {
                    id: insideFrame_instanceVariation
                    text: "insideFrame_instanceVariation"
                    StyleVariation.variations: ["instanceVariation"]
                }
            }
        }
    }
}
