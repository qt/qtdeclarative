import Qt.v4 1.0

Conversion {
    // test assigning float prop to other proptypes.
    boolProp: floatProp
    intProp: floatProp
    floatProp: 4.4
    doubleProp: floatProp
    qrealProp: floatProp
    qstringProp: floatProp
    qurlProp: floatProp
    vec3Prop: Qt.vector3d(floatProp, floatProp, floatProp)
}
