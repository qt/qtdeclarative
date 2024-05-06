import QtQuick
import QtQuick.VectorImage

Rectangle{
    id: topLevelItem
    width: 200
    height: 880

    Column {
        anchors.fill: parent
        Repeater {
            model: ListModel {
                ListElement {
                    name: "Stretch"
                    mode: VectorImage.Stretch
                }
                ListElement {
                    name: "NoResize"
                    mode: VectorImage.NoResize
                }
                ListElement {
                    name: "PreserveAspectCrop"
                    mode: VectorImage.PreserveAspectCrop
                }
                ListElement {
                    name: "PreserveAspectFit"
                    mode: VectorImage.PreserveAspectFit
                }

            }

            Column {
                width: 200
                height: 200 + t.height
                Rectangle {
                    color: "white"
                    border.width: 1
                    border.color: "black"
                    width: 152
                    height: 202
                    VectorImage {
                        x: 1
                        y: 1
                        width: 150
                        height: 200
                        source: "../shared/qt_logo.svg"
                        fillMode: mode
                        clip: true
                        z: 100
                    }
                }
                Text {
                    id: t
                    text: name
                }
            }
        }
    }
}
