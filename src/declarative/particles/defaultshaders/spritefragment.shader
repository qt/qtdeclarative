uniform sampler2D texture;
uniform sampler2D colortable;
uniform sampler2D opacitytable;
uniform lowp float qt_Opacity;

varying highp vec2 fTexA;
varying highp vec2 fTexB;
varying lowp float progress;
varying lowp vec4 fColor;
varying lowp float tt;

void main() {
    gl_FragColor = mix(texture2D(texture, fTexA), texture2D(texture, fTexB), progress)
            * fColor
            * texture2D(colortable, vec2(tt, 0.5))
            * (texture2D(opacitytable, vec2(tt, 0.5)).w * qt_Opacity);
}
