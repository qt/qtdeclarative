import Qt.v4 1.0

Conversion {
    // test assigning int prop to other proptypes.
    boolProp: intProp
    intProp: 4
    floatProp: intProp
    doubleProp: intProp
    qrealProp: intProp
    qstringProp: intProp
    qurlProp: intProp
    vec3Prop: Qt.vector3d(intProp, intProp, intProp)
}
