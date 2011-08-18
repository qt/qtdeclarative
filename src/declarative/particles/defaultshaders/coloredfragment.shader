uniform sampler2D texture;
uniform lowp float qt_Opacity;

varying highp vec2 fTex;
varying lowp vec4 fColor;

void main() {
    gl_FragColor = (texture2D(texture, fTex)) * fColor * qt_Opacity;
}

