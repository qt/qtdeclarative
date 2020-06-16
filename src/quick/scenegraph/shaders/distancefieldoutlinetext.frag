varying highp vec2 sampleCoord;

uniform sampler2D _qt_texture;
uniform lowp vec4 color;
uniform lowp vec4 styleColor;
uniform mediump float alphaMin;
uniform mediump float alphaMax;
uniform mediump float outlineAlphaMax0;
uniform mediump float outlineAlphaMax1;

void main()
{
    mediump float d = texture2D(_qt_texture, sampleCoord).a;
    mediump float a = smoothstep(alphaMin, alphaMax, d);
    gl_FragColor = (1.0 - a) * styleColor * smoothstep(outlineAlphaMax0, outlineAlphaMax1, d);
}
