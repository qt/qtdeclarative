import QtQml

QtObject {
    id: self
    property QtObject b
    property Component c
    function a() : TypeAnnotationCycle2 { return c.createObject() as TypeAnnotationCycle2 }

    Component.onCompleted: {
        c = Qt.createComponent("TypeAnnotationCycle2.qml");
        let v = a();
        v.addTypeAnnotationCycle1(self as TypeAnnotationCycle1);
        b = v.b;
    }
}
