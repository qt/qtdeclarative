import QtQuick 2.0

Item
{
    width: 100
    height: 100

    Rectangle {
        id: box
        width: 100
        height: 100

        color: "#0000ff"

        Rectangle {
            x: 50
            width: 50
            height: 100
            color: "#00ff00"
        }

        layer.enabled: true
        layer.effect: ShaderEffect {
            fragmentShader: "
            uniform lowp sampler2D source;
            uniform lowp float qt_Opacity;
            varying highp vec2 qt_TexCoord0;
            void main() {
                gl_FragColor = texture2D(source, qt_TexCoord0).bgra * qt_Opacity;
            }"
        }

    }
}
