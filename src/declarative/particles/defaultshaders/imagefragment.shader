uniform sampler2D texture;
uniform lowp float qt_Opacity;

#ifdef SPRITE
varying highp vec4 fTexS;
#else
#ifdef DEFORM //First non-pointsprite
varying highp vec2 fTex;
#endif
#endif
#ifdef COLOR
varying lowp vec4 fColor;
#else
varying lowp float fFade;
#endif
#ifdef TABLE
varying lowp vec2 tt;
uniform sampler2D colortable;
uniform sampler2D opacitytable;
uniform sampler2D sizetable;
#endif

void main() {
#ifdef SPRITE
    gl_FragColor = mix(texture2D(texture, fTexS.xy), texture2D(texture, fTexS.zw), tt.y)
            * fColor
            * texture2D(colortable, tt)
            * (texture2D(opacitytable, tt).w * qt_Opacity);
#else
#ifdef TABLE
    highp vec2 tex = (((fTex - 0.5) / texture2D(sizetable, tt).w) + 0.5);
    lowp vec4 color;
    if(tex.x < 1.0 && tex.x > 0.0 && tex.y < 1.0 && tex.y > 0.0){//No CLAMP_TO_BORDER in ES2, so have to do it ourselves
        color = texture2D(texture, tex);//TODO: Replace with uniform array in vertex shader
    }else{
        color = vec4(0.,0.,0.,0.);
    }
    gl_FragColor = color
            * fColor
            * texture2D(colortable, tt)
            * (texture2D(opacitytable,tt).w * qt_Opacity);
#else
#ifdef DEFORM
    gl_FragColor = (texture2D(texture, fTex)) * fColor * qt_Opacity;
#else
#ifdef COLOR
    gl_FragColor = (texture2D(texture, gl_PointCoord)) * fColor * qt_Opacity;
#else
    gl_FragColor = texture2D(texture, gl_PointCoord) * (fFade * qt_Opacity);
#endif //COLOR
#endif //DEFORM
#endif //TABLE
#endif //SPRITE
}
