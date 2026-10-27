import QtQml
import OpaqueSequence

QtObject {
    property Derived d
    function f() {
        d.indexes().sort((a, b) => 0)
    }
}
