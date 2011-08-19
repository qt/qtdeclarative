uniform sampler2D texture;
uniform lowp float qt_Opacity;

varying lowp float fFade;

void main() {
    gl_FragColor = texture2D(texture, gl_PointCoord) * (fFade * qt_Opacity);
}
