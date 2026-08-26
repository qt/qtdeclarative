import QtQml
QtObject {
    id: root
    component C : QtObject {
        property int i
    }
    property C c: C {}
    root.c.i: 4
}
