uniform sampler2D texture;
uniform lowp float qt_Opacity;

varying lowp vec4 fColor;

void main() {
    gl_FragColor = (texture2D(texture, gl_PointCoord)) * fColor * qt_Opacity;
}

