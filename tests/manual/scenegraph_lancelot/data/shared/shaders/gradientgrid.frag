uniform lowp sampler2D source;
varying highp vec2 qt_TexCoord0;
uniform lowp float qt_Opacity;
void main() {
    lowp float r = mod(qt_TexCoord0.x * 10.0, 1.0);
    lowp float g = mod(qt_TexCoord0.y * 10.0, 1.0);
    lowp float b = qt_TexCoord0.x;
    gl_FragColor = vec4(r, g, b, 1) * texture2D(source, qt_TexCoord0).a;
}
