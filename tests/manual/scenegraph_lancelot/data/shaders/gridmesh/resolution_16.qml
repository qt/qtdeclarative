import QtQuick 2.0

Rectangle {
    width: 320
    height: 480
    color: "skyblue"
    ShaderEffect {
        anchors.centerIn: parent
        width: 22 * 12
        height: 16 * 12
        property variant source: ShaderEffectSource {
            sourceItem: Rectangle {
                width: 22 * 20
                height: 16 * 20
                color: "#EF2B2D"
                Rectangle {
                    y: 6 * 20
                    height: 4 * 20
                    width: 22 * 20
                    color: "white"
                }
                Rectangle {
                    x: 6 * 20
                    width: 4 * 20
                    height: 16 * 20
                    color: "white"
                }
                Rectangle {
                    y: 7 * 20
                    height: 2 * 20
                    width: 22 * 20
                    color: "#002868"
                }
                Rectangle {
                    x: 7 * 20
                    width: 2 * 20
                    height: 16 * 20
                    color: "#002868"
                }
            }
            smooth: true
        }
        vertexShader: "qrc:shaders/wave.vert"
        mesh: GridMesh {
            property int r: 16
            resolution: Qt.size(r, r)
        }
    }
}
