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
#endif

void main() {
#ifdef SPRITE
    gl_FragColor = mix(texture2D(texture, fTexS.xy), texture2D(texture, fTexS.zw), tt.y)
            * fColor
            * texture2D(colortable, tt)
            * qt_Opacity;
#else
#ifdef TABLE
    gl_FragColor = texture2D(texture, fTex)
            * fColor
            * texture2D(colortable, tt)
            * qt_Opacity;
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
