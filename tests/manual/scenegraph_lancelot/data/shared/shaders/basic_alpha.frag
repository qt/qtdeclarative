varying highp vec2 qt_TexCoord0;
uniform lowp float qt_Opacity;
void main() {
    lowp float u = qt_TexCoord0.x;
    lowp float v = qt_TexCoord0.y;
    gl_FragColor = vec4(u*v, v*v, v, v);
}
