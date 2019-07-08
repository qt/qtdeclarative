#version 440

layout(location = 0) in vec4 vertices;

layout(location = 0) out vec2 coords;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    gl_Position = vertices;
    coords = vertices.xy;
}
