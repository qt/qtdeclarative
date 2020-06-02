import QtQuick 2.0
import ShaderEffectTest 1.0

TestShaderEffect {
    width:100;
    height:100;
    fragmentShader: "qrc:/data/test.frag"
    vertexShader: "qrc:/data/test.vert"
}
