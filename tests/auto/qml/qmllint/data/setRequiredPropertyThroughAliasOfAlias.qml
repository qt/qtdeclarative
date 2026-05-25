import QtQml

QtObject {
    id: root

    component C : QtObject {
        id: c
        required property int i
        property alias ai: c.i
    }

    component C2 : C {
        id: c2
        property alias aai: c2.ai
    }

    property C2 c2: C2 {
        aai: 1
    }
}
