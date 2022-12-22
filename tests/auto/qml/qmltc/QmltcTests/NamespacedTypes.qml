import QtQml
import QmltcTests

TypeWithSubnamespace {
    property TypeWithNamespace myType: TypeWithNamespace {}
    property QtObject myObject : QtObject {
        property int value: 123
    }
    function f() {
        myObject.value = x
    }
}
