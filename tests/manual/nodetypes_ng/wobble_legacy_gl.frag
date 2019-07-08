uniform sampler2D source;
uniform highp float amplitude;
uniform highp float frequency;
uniform highp float time;
uniform lowp float qt_Opacity;
varying highp vec2 qt_TexCoord0;
void main() {
    highp vec2 p = sin(time + frequency * qt_TexCoord0);
    gl_FragColor = texture2D(source, qt_TexCoord0 + amplitude * vec2(p.y, -p.x)) * qt_Opacity;
}
