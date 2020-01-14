varying highp vec2 qt_TexCoord0;
uniform sampler2D frontSource;
uniform sampler2D backSource;
uniform lowp float qt_Opacity;
void main() {
    gl_FragColor = gl_FrontFacing
                 ? texture2D(frontSource, qt_TexCoord0)
                 : texture2D(backSource, qt_TexCoord0);
}
