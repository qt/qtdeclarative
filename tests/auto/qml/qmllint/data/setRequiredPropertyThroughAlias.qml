import QtQml

QtObject {
    id: root

    component C : QtObject {
        required property int i
    }

    component C2 : C {
        id: c2
        property alias ai: c2.i
    }

    property C2 c2: C2 {
        ai: 1
    }
}
