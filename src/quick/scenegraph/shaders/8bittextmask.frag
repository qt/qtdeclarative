varying highp vec2 sampleCoord;

uniform lowp sampler2D texture;
uniform lowp vec4 color;

void main()
{
    gl_FragColor = color * texture2D(texture, sampleCoord).a;
}