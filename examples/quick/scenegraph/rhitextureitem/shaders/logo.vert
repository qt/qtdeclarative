#version 440

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec4 v_color;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
};

void main()
{
    vec3 toLight = normalize(vec3(0.0, 0.3, 1.0));
    float angle = max(dot(normal, toLight), 0.0);
    vec3 col = vec3(0.40, 1.0, 0.0);
    v_color = clamp(vec4(col * 0.2 + col * 0.8 * angle, 1.0), 0.0, 1.0);
    gl_Position = matrix * vertex;
}
