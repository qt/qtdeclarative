import QtQuick 2.0

Rectangle {
    width: 200
    height: 200
    color: "blue"
    layer.enabled: true
    layer.effect: ShaderEffect {
        fragmentShader: "samplerNameChange.frag.qsb"
    }
    Component.onCompleted: layer.samplerName = "foo"
}
