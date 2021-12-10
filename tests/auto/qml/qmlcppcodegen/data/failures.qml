import QtQml
import TestTypes
import Ambiguous 1.2

QtObject {
    property string attachedForNonObject: objectName.Component.objectName
    property string attachedForNasty: Nasty.objectName

    property Nasty nasty: Nasty {
        objectName: Component.objectName
    }

    onFooBar: console.log("nope")

    function doesReturnValue() { return 5; }

    property Thing thing: Thing {
        property int b: a + 1
    }

    property NotHere here: NotHere {
        property int c: b + 1
    }
}
