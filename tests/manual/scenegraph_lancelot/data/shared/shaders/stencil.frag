uniform lowp sampler2D maskSource;
uniform lowp sampler2D colorSource;
varying highp vec2 qt_TexCoord0;
uniform lowp float qt_Opacity;
void main() {
    gl_FragColor = texture2D(maskSource, qt_TexCoord0).a * texture2D(colorSource, qt_TexCoord0.yx);
}
