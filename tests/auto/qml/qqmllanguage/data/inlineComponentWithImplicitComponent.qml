import QtQml

QtObject {
    component C1: QtObject {
        property Component comp: null
    }

    component C2: C1 {
        comp: QtObject {
            objectName: "green"
        }
    }

    component C3: C1 {
        comp: Component {
            QtObject {
                objectName: "blue"
            }
        }
    }

    property QtObject c1: C1 {}
    property QtObject c2: C2 {}
    property QtObject c3: C3 {}

    objectName: c2.comp.createObject().objectName + " " + c3.comp.createObject().objectName
}
