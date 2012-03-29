import Qt.v4 1.0

Conversion {
    // test assigning vector prop to other proptypes.
    boolProp: vec3Prop
    intProp: vec3Prop
    floatProp: vec3Prop
    doubleProp: vec3Prop
    qrealProp: vec3Prop
    qstringProp: vec3Prop
    qurlProp: vec3Prop
    vec3Prop: Qt.vector3d(4, 4, 4)
}
