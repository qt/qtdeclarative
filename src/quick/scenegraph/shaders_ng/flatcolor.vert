#version 440

layout(location = 0) in vec4 vertexCoord;

layout(std140, binding = 0) uniform buf {
#if QSHADER_VIEW_COUNT >= 2
    mat4 matrix[QSHADER_VIEW_COUNT];
#else
    mat4 matrix;
#endif
    vec4 color;
};

void main()
{
#if QSHADER_VIEW_COUNT >= 2
    gl_Position = matrix[gl_ViewIndex] * vertexCoord;
#else
    gl_Position = matrix * vertexCoord;
#endif
}
