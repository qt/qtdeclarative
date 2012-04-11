import Qt.v4 1.0

Conversion {
    // test assigning qreal prop to other prop types
    boolProp: qrealProp
    intProp: qrealProp
    floatProp: qrealProp
    doubleProp: qrealProp
    qrealProp: 4.44
    qstringProp: qrealProp
    qurlProp: qrealProp
    vec3Prop: Qt.vector3d(qrealProp, qrealProp, qrealProp)
}
