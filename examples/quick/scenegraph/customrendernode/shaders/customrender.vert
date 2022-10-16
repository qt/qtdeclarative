#version 440

layout(location = 0) in vec2 vertex;

layout(location = 0) out vec3 color;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
};

out gl_PerVertex { vec4 gl_Position; };

void main()
{

    switch (gl_VertexIndex%3) {
    case 0:
        color = vec3(1.0, 0.0, 0.0);
        break;
    case 1:
        color = vec3(0.0, 1.0, 0.0);
        break;
    case 2:
        color = vec3(0.0, 0.0, 1.0);
        break;
    }

    gl_Position = qt_Matrix * vec4(vertex, 0.0, 1.0);

}
