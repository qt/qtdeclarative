uniform lowp sampler2D source;
varying highp vec2 qt_TexCoord0;

void main() {
    lowp vec4 c = texture2D(source, qt_TexCoord0);
    lowp float level = c.r * 0.3 + c.g * 0.59 + c.b * 0.11;

    gl_FragColor = vec4(level, level, level, c.a);
}
