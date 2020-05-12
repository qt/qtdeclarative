import QtQuick 2.15

Item {
    width: 320
    height: 480

    Repeater {
        model: 5
        Rectangle {
            x: 10 + 30 * index
            y: 10 + 30 * index
            width: 100
            height: 100
            color: Qt.hsva(index/5.0, 1.0, 1.0, 0.5)
        }
    }

    layer.enabled: true
    layer.samplerName: "source"
    layer.effect: ShaderEffect {
        fragmentShader: "qrc:shaders/desaturate.frag"
    }
}
