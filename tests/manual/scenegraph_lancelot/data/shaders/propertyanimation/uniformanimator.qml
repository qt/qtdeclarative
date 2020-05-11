import QtQuick 2.15

Item {
    width: 320
    height: 480

    ShaderEffect {
        x: 10
        y: 250
        width: 300
        height: 200
        property real colorProperty: 0.0
        fragmentShader: "qrc:shaders/property.frag"

        UniformAnimator on colorProperty {
            duration: 200
            from: 0.0
            to: 1.0
        }
    }
}
