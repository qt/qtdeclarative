import Qt.v4 1.0

Conversion {
    // test assigning url prop to other proptypes.
    boolProp: qurlProp
    intProp: qurlProp
    floatProp: qurlProp
    doubleProp: qurlProp
    qrealProp: qurlProp
    qstringProp: qurlProp
    qurlProp: "4"
    vec3Prop: Qt.vector3d(qurlProp, qurlProp, qurlProp)
}
