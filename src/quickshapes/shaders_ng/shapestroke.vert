#version 440

layout(location = 0) in vec4 vertexCoord;
layout(location = 1) in vec2 inA;
layout(location = 2) in vec2 inB;
layout(location = 3) in vec2 inC;
layout(location = 4) in vec2 normalVector;

layout(location = 0) out vec4 P;
layout(location = 1) out vec2 A;
layout(location = 2) out vec2 B;
layout(location = 3) out vec2 C;
layout(location = 4) out vec2 HG;
layout(location = 5) out float offset;


layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;

    float matrixScale;
    float opacity;
    float reserved2;
    float reserved3;

    vec4 strokeColor;

    float strokeWidth;
    float debug;
    float reserved5;
    float reserved6;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

#define SQRT2 1.41421356237

float qdot(vec2 a, vec2 b)
{
    return a.x * b.x + a.y * b.y;
}

void main()
{
    P = vertexCoord + vec4(normalVector, 0.0, 0.0) * SQRT2/ubuf.matrixScale;

    A = inA;
    B = inB;
    C = inC;

    // Find the parameters H, G for the depressed cubic
    // t^2+H*t+G=0
    // that results from the equation
    // Q'(s).(p-Q(s)) = 0
    // The last parameter is the static offset between s and t:
    // s = t - b/(3a)
    // use it to get back the parameter t

    // this is a constant for the curve
    float a = -2. * qdot(A, A);
    // this is a constant for the curve
    float b = -3. * qdot(A, B);
    //this is linear in p so it can be put into the shader with vertex data
    float c = 2. * qdot(A, P.xy) - qdot(B, B) - 2. * qdot(A, C);
    //this is linear in p so it can be put into the shader with vertex data
    float d = qdot(B, P.xy) - qdot(B, C);
    // convert to depressed cubic.
    // both functions are linear in c and d and thus linear in p
    float H = (3. * a * c - b * b) / (3. * a * a);
    float G = (2. * b * b * b - 9. * a * b * c + 27. * a * a * d) / (27. * a * a * a);
    HG = vec2(H, G);
    offset = b/(3*a);



    gl_Position = ubuf.qt_Matrix * P;
}
