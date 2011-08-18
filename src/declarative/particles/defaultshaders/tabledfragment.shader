uniform sampler2D texture;
uniform sampler2D colortable;
uniform sampler2D opacitytable;
uniform sampler2D sizetable;
uniform lowp float qt_Opacity;

varying highp vec2 fTex;
varying lowp vec4 fColor;
varying lowp float tt;

void main() {
    highp vec2 tex = (((fTex - 0.5) / texture2D(sizetable, vec2(tt, 0.5)).w) + 0.5);
    lowp vec4 color;
    if(tex.x < 1.0 && tex.x > 0.0 && tex.y < 1.0 && tex.y > 0.0){//No CLAMP_TO_BORDER in ES2, so have to do it ourselves
        color = texture2D(texture, tex);
    }else{
        color = vec4(0.,0.,0.,0.);
    }
    gl_FragColor = color
            * fColor
            * texture2D(colortable, vec2(tt, 0.5))
            * (texture2D(opacitytable, vec2(tt, 0.5)).w * qt_Opacity);
}
