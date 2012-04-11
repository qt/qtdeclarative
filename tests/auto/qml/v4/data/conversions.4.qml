import Qt.v4 1.0

Conversion {
    // test assigning double prop to other prop types
    boolProp: doubleProp
    intProp: doubleProp
    floatProp: doubleProp
    doubleProp: 4.444444444
    qrealProp: doubleProp
    qstringProp: doubleProp
    qurlProp: doubleProp
    vec3Prop: Qt.vector3d(doubleProp, doubleProp, doubleProp)
}
