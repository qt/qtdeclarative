import Qt.v4 1.0

Conversion {
    // test assigning string prop to other proptypes.
    boolProp: qstringProp
    intProp: qstringProp
    floatProp: qstringProp
    doubleProp: qstringProp
    qrealProp: qstringProp
    qstringProp: "4"
    qurlProp: qstringProp
    vec3Prop: Qt.vector3d(qstringProp, qstringProp, qstringProp)
}
