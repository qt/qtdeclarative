#version 440

layout(location = 0) in vec4 vertexCoord;
layout(location = 1) in vec4 vertexTexCoord;
layout(location = 2) in vec4 vertexGradient;
layout(location = 3) in vec2 normalVector;

layout(location = 0) out vec4 qt_TexCoord;
layout(location = 1) out vec4 gradient;

#if defined(LINEARGRADIENT)
layout(location = 2) out float gradTabIndex;
#elif defined(RADIALGRADIENT) || defined(CONICALGRADIENT)
layout(location = 2) out vec2 coord;
#endif

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float matrixScale;
    float opacity;
    float debug;
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

#define SQRT2 1.41421356237

vec4 addOffset(vec4 texCoord, vec2 offset, vec4 duvdxy)
{
    float dudx = duvdxy.x;
    float dvdx = duvdxy.y;
    float dudy = duvdxy.z;
    float dvdy = duvdxy.w;
    float u = offset.x * dudx + offset.y * dudy;
    float v = offset.x * dvdx + offset.y * dvdy;
    // special case external triangles for concave curves
    int specialCase = int(texCoord.z > 0) * (int(offset.x != 0) + int(offset.y != 0));
    return vec4(texCoord.x + u, texCoord.y + v, texCoord.z, float(specialCase));
}

void main()
{
    vec2 offset = normalVector * SQRT2/ubuf.matrixScale;

    qt_TexCoord = addOffset(vertexTexCoord, offset, vertexGradient);

    gradient = vertexGradient / ubuf.matrixScale;

#if defined(LINEARGRADIENT)
    vec2 gradVec = ubuf.gradientEnd - ubuf.gradientStart;
    gradTabIndex = dot(gradVec, vertexCoord.xy - ubuf.gradientStart.xy) / dot(gradVec, gradVec);
#elif defined(RADIALGRADIENT) || defined(CONICALGRADIENT)
    coord = vertexCoord.xy - ubuf.translationPoint;
#endif

    gl_Position = ubuf.qt_Matrix * (vertexCoord + vec4(offset, 0, 0));
}
