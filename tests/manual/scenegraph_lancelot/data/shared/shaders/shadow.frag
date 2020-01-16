uniform lowp sampler2D source;
uniform highp vec2 offset;
uniform highp vec2 delta;
varying highp vec2 qt_TexCoord0;
uniform lowp float qt_Opacity;
void main() {
    highp vec2 delta2 = vec2(delta.x, -delta.y);
    lowp float shadow = 0.25 * (texture2D(source, qt_TexCoord0 - offset + delta).a
                      + texture2D(source, qt_TexCoord0 - offset - delta).a
                      + texture2D(source, qt_TexCoord0 - offset + delta2).a
                      + texture2D(source, qt_TexCoord0 - offset - delta2).a);
    lowp vec4 color = texture2D(source, qt_TexCoord0);
    gl_FragColor = mix(vec4(vec3(0.), 0.5 * shadow), color, color.a);
}
