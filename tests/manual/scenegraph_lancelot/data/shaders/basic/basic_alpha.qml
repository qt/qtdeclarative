import QtQuick 2.0

Item {
    width: 320
    height: 480

    Rectangle {
        color: "darkGray"
        x: 100
        y: 100
        width: 100
        height: 200
    }

    ShaderEffect {
        x: 10
        y: 10
        width: 300
        height: 200
        fragmentShader: "qrc:shaders/basic_alpha.frag"
        blending: true
    }

    ShaderEffect {
        x: 10
        y: 250
        width: 300
        height: 200
        fragmentShader: "qrc:shaders/basic_alpha.frag"
        blending: false
    }

}
