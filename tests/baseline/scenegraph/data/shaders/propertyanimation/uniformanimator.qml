import QtQuick

Item {
    width: 320
    height: 480
    property bool suspendGrabbing: animator.running

    ShaderEffect {
        x: 10
        y: 250
        width: 300
        height: 200
        property real colorProperty: 0.0
        fragmentShader: "qrc:shaders/property.frag.qsb"

        UniformAnimator on colorProperty {
            id: animator
            duration: 20
            from: 0.0
            to: 1.0
        }
    }
}
