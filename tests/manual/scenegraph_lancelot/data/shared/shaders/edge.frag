uniform lowp sampler2D source;
varying highp vec2 qt_TexCoord0;
uniform highp vec2 textureSize;
uniform lowp vec4 color;
uniform lowp float qt_Opacity;
void main() {
    highp vec2 dx = vec2(0.5 / textureSize.x, 0.);
    highp vec2 dy = vec2(0., 0.5 / textureSize.y);
    gl_FragColor = color * 0.25
                 * (texture2D(source, qt_TexCoord0 + dx + dy).a
                 + texture2D(source, qt_TexCoord0 + dx - dy).a
                 + texture2D(source, qt_TexCoord0 - dx + dy).a
                 + texture2D(source, qt_TexCoord0 - dx - dy).a);
}
