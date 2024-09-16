#version 440

layout(location = 0) in vec4 P;
layout(location = 1) in vec2 A;
layout(location = 2) in vec2 B;
layout(location = 3) in vec2 C;
layout(location = 4) in vec2 HG;
layout(location = 5) in float offset;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
#if QSHADER_VIEW_COUNT >= 2
    mat4 qt_Matrix[QSHADER_VIEW_COUNT];
#else
    mat4 qt_Matrix;
#endif

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

float cuberoot(float x)
{
    return sign(x) * pow(abs(x), 1 / 3.);
}

#define PI 3.1415926538

vec3 solveDepressedCubic(float p, float q)
{
    float D = q * q / 4. + p * p * p / 27.;

    float u1 = cuberoot(-q / 2. - sqrt(D));
    float u2 = cuberoot(-q / 2. + sqrt(D));
    vec3 rootsD1 = vec3(u1 - p / (3. * u1), u2 - p / (3. * u2), 0);

    float v = 2.*sqrt(-p / 3.);
    float t = acos(3. * q / p / v) / 3.;
    float k = 2. * PI / 3.;
    vec3 rootsD2 = vec3(v * cos(t), v * cos(t - k), v * cos(t - 2. * k));

    return D > 0 ? rootsD1 : rootsD2;
}

mat2 qInverse(mat2 matrix) {
    float a = matrix[0][0], b = matrix[0][1];
    float c = matrix[1][0], d = matrix[1][1];

    float determinant = a * d - b * c;
    float invDet = 1.0 / determinant;

    mat2 inverseMatrix;
    inverseMatrix[0][0] = d * invDet;
    inverseMatrix[0][1] = -b * invDet;
    inverseMatrix[1][0] = -c * invDet;
    inverseMatrix[1][1] = a * invDet;

    return inverseMatrix;
}

void main()
{
    vec3 s = solveDepressedCubic(HG.x, HG.y) - vec3(offset, offset, offset);

    vec2 Qmin = vec2(1e10, 1e10);
    float dmin = 1e4;
    for (int i = 0; i < 3; i++) {
        float t = clamp(s[i], 0., 1.);
        vec2 Q = A * t * t + B * t + C;
        float d = length(Q - P.xy);
        float foundNewMin = step(d, dmin);
        dmin = min(d, dmin);
        Qmin = foundNewMin * Q + (1. - foundNewMin) * Qmin;
    }
    vec2 n = (P.xy - Qmin) / dmin;
    vec2 Q1 = (Qmin + ubuf.strokeWidth / 2. * n);
    vec2 Q2 = (Qmin - ubuf.strokeWidth / 2. * n);

    // Converting to screen coordinates:
#if defined(USE_DERIVATIVES)
    mat2 T = mat2(dFdx(P.x), dFdy(P.x), dFdx(P.y), dFdy(P.y));
    mat2 Tinv = qInverse(T);
    vec2 Q1_s = Tinv * Q1;
    vec2 Q2_s = Tinv * Q2;
    vec2 P_s = Tinv * P.xy;
    vec2 n_s = Tinv * n;
    n_s = n_s / length(n_s);
#else
    vec2 Q1_s = ubuf.matrixScale * Q1;
    vec2 Q2_s = ubuf.matrixScale * Q2;
    vec2 P_s = ubuf.matrixScale * P.xy;
    vec2 n_s = n;
#endif

    // Geometric solution for anti aliasing using the known distances
    // to the edges of the path in the screen coordinate system.
    float dist1 = dot(P_s - Q1_s, n_s);
    float dist2 = dot(P_s - Q2_s, n_s);

    // Calculate the fill coverage if the line is crossing the square cell
    // normally (vertically or horizontally).
    // float fillCoverageLin = clamp(0.5-dist1, 0., 1.) - clamp(0.5-dist2, 0., 1.);

    // Calculate the fill coverage if the line is crossing the square cell
    // diagonally.
    float fillCoverageDia = clamp(step(0., -dist1) + sign(dist1) * pow(max(0., sqrt(2.) / 2. - abs(dist1)), 2.), 0., 1.) -
                            clamp(step(0., -dist2) + sign(dist2) * pow(max(0., sqrt(2.) / 2. - abs(dist2)), 2.), 0., 1.);

    // Merge the normal and the diagonal solution. The merge factor is periodic
    // in 90 degrees and 0/1 at 0 and 45 degree. The simple equation was
    // estimated after numerical experiments.
    // float mergeFactor = 2 * abs(n_s.x) * abs(n_s.y);
    // float fillCoverage = mergeFactor * fillCoverageDia + (1-mergeFactor) * fillCoverageLin;

    // It seems to be sufficient to use the equation for the diagonal.
    float fillCoverage = fillCoverageDia;

    // The center line is sometimes not filled because of numerical issues. This fixes this.
    float centerline = step(ubuf.strokeWidth * 0.01, dmin);
    fillCoverage = fillCoverage * centerline + min(1., ubuf.strokeWidth * ubuf.matrixScale) * (1. - centerline);

    fragColor = vec4(ubuf.strokeColor.rgb, 1.0) *ubuf.strokeColor.a * fillCoverage * ubuf.opacity
                + ubuf.debug * vec4(0.0, 0.5, 1.0, 1.0) * (1.0 - fillCoverage) * ubuf.opacity;
}
