import QtQuick 2.0

Item {
    width: 320
    height: 480

    ShaderEffect {
        anchors.fill: parent;
        fragmentShader: "qrc:shaders/basic.frag"
    }
}
