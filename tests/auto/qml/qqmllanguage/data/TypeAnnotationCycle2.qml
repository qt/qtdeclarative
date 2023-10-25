import QtQml

QtObject {
    property QtObject b
    function addTypeAnnotationCycle1(c: TypeAnnotationCycle1) { b = c; }
}
