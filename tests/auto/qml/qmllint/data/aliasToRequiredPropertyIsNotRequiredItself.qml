import QtQml

QtObject {
    // Not at the root level
    property QtObject o1: QtObject {
        component Comp1 : QtObject {
            required property int i1
        }

        property Comp1 c1: Comp1 {
            id: compId1
            i1: 1
        }

        property alias aliasToRequired1: compId1.i1
    }


    // At the root level
    component Comp2 : QtObject {
        required property int i2
    }

    property Comp2 c2: Comp2 {
        id: compId2
        i2: 2
    }

    property alias aliasToRequired2: compId2.i2
}
