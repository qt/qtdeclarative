import QtQuick 2.0

Item {
    width: 320
    height: 480

    Image {
        id: image;
        source: "face-smile.png"
        visible: false
    }

    ShaderEffect {
        anchors.fill: image
        property variant source: image
        fragmentShader: "qrc:shaders/gradient2.frag"
    }
}
