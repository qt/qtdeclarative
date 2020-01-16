uniform highp mat4 qt_Matrix;
attribute highp vec4 qt_Vertex;
attribute highp vec2 qt_MultiTexCoord0;
varying highp vec2 qt_TexCoord0;

void main() {
    highp vec4 pos = qt_Vertex;
    pos.x += sin(qt_Vertex.y * 0.02) * 20.;
    pos.y += sin(qt_Vertex.x * 0.02) * 20.;
    gl_Position = qt_Matrix * pos;
    qt_TexCoord0 = qt_MultiTexCoord0;
}
