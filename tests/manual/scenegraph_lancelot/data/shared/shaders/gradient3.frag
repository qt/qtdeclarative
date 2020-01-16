uniform lowp sampler2D source;
varying highp vec2 qt_TexCoord0;
uniform lowp float qt_Opacity;
void main() {
    gl_FragColor = vec4(0, qt_TexCoord0.y, 1, 1) * texture2D(source, qt_TexCoord0).a;
}
