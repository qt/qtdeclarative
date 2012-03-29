import Qt.v4 1.0

Conversion {
    // test assigning bool prop to other proptypes.
    boolProp: true
    intProp: boolProp
    floatProp: boolProp
    doubleProp: boolProp
    qrealProp: boolProp
    qstringProp: boolProp
    qurlProp: boolProp
    vec3Prop: Qt.vector3d(boolProp, boolProp, boolProp)
}
