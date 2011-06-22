import QtQuick 2.0

ShaderEffectItem {
    id: shadow

    property Item sourceItem

    property variant delta: Qt.size(0.0, 1.0 / height)
    property variant source: ShaderEffectSource {
        smooth: true
        sourceItem: ShaderEffectItem {
            width: shadow.width
            height: shadow.height
            property variant delta: Qt.size(1.0 / width, 0.0)
            property variant source: ShaderEffectSource {
                id: shadowSource
                sourceItem: shadow.sourceItem
                smooth: true
            }
            fragmentShader: "
                uniform sampler2D source;
                uniform highp vec2 delta;
                varying highp vec2 qt_TexCoord0;
                void main() {
                    gl_FragColor = 0.0538 * texture2D(source, qt_TexCoord0 - 3.182 * delta)
                                 + 0.3229 * texture2D(source, qt_TexCoord0 - 1.364 * delta)
                                 + 0.2466 * texture2D(source, qt_TexCoord0)
                                 + 0.3229 * texture2D(source, qt_TexCoord0 + 1.364 * delta)
                                 + 0.0538 * texture2D(source, qt_TexCoord0 + 3.182 * delta);
                }"
        }
    }
    fragmentShader: "
        uniform sampler2D source;
        uniform highp vec2 delta;
        varying highp vec2 qt_TexCoord0;
        void main() {
            gl_FragColor = 0.0538 * texture2D(source, qt_TexCoord0 - 3.182 * delta)
                         + 0.3229 * texture2D(source, qt_TexCoord0 - 1.364 * delta)
                         + 0.2466 * texture2D(source, qt_TexCoord0)
                         + 0.3229 * texture2D(source, qt_TexCoord0 + 1.364 * delta)
                         + 0.0538 * texture2D(source, qt_TexCoord0 + 3.182 * delta);
        }"
}
