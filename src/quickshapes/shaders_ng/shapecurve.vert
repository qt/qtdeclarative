#version 440

layout(location = 0) in vec4 vertexCoord;
layout(location = 1) in vec4 vertexTexCoord;
layout(location = 2) in vec4 vertexDebugColor;
layout(location = 3) in vec4 vertexGradient;

layout(location = 0) out vec4 qt_TexCoord;
layout(location = 1) out vec4 debugColor;

#if defined(LINEARGRADIENT)
layout(location = 2) out float gradTabIndex;
#  define NEXT_LOCATION 3
#elif defined(RADIALGRADIENT) || defined(CONICALGRADIENT)
layout(location = 2) out vec2 coord;
#  define NEXT_LOCATION 3
#else
#  define NEXT_LOCATION 2
#endif

layout(location = NEXT_LOCATION) out vec4 gradient;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float matrixScale;
    float opacity;
    float reserved2;
    float reserved3;

#if defined(STROKE)
    vec4 strokeColor;
    float strokeWidth;
    float reserved4;
    float reserved5;
    float reserved6;
#endif

#if defined(LINEARGRADIENT)
    vec2 gradientStart;
    vec2 gradientEnd;
#elif defined(RADIALGRADIENT)
    vec2 translationPoint;
    vec2 focalToCenter;
    float centerRadius;
    float focalRadius;
#elif defined(CONICALGRADIENT)
    vec2 translationPoint;
    float angle;
#else
    vec4 color;
#endif
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    qt_TexCoord = vertexTexCoord;
    debugColor = vertexDebugColor;
    gradient = vertexGradient / ubuf.matrixScale;

#if defined(LINEARGRADIENT)
    vec2 gradVec = ubuf.gradientEnd - ubuf.gradientStart;
    gradTabIndex = dot(gradVec, vertexCoord.xy - ubuf.gradientStart.xy) / dot(gradVec, gradVec);
#elif defined(RADIALGRADIENT) || defined(CONICALGRADIENT)
    coord = vertexCoord.xy - ubuf.translationPoint;
#endif

    gl_Position = ubuf.qt_Matrix * vertexCoord;
}
