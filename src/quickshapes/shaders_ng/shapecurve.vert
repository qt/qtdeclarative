#version 440

layout(location = 0) in vec4 vertexCoord;
layout(location = 1) in vec3 vertexTexCoord;
layout(location = 2) in vec4 vertexDebugColor;

layout(location = 0) out vec3 qt_TexCoord;
layout(location = 1) out vec4 debugColor;

#if defined(LINEARGRADIENT)
layout(location = 2) out float gradTabIndex;
#elif defined(RADIALGRADIENT) || defined(CONICALGRADIENT)
layout(location = 2) out vec2 coord;
#endif

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;

#if defined(STROKE)
    vec4 strokeColor;
    float strokeWidth;
    float reserved1;
    float reserved2;
    float reserved3;
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

#if defined(LINEARGRADIENT)
    vec2 gradVec = ubuf.gradientEnd - ubuf.gradientStart;
    gradTabIndex = dot(gradVec, vertexCoord.xy - ubuf.gradientStart.xy) / dot(gradVec, gradVec);
#elif defined(RADIALGRADIENT) || defined(CONICALGRADIENT)
    coord = vertexCoord.xy - ubuf.translationPoint;
#endif

    gl_Position = ubuf.qt_Matrix * vertexCoord;
}
