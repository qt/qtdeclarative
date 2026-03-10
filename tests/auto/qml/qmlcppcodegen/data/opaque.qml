pragma Strict
import QtQml
import TestTypes

QtObject {
    id: self

    property Secretive s: Secretive {
        id: s1
    }

    property Secretive dummy: Secretive {
        id: s2

        function opaqueArgument() : void {  s1.takesOpaque(s1.opaque_prop) }

        function opaqueReturn() : void {
            s1.takesOpaque(s2.returnsOpaque())
        }
    }

    Component.onCompleted: {
        s2.opaqueArgument()
        s2.opaqueReturn()
    }
}
