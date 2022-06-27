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

    property Thing2 thing2: Thing2 {
        property int b: a + 2
    }

    property NotHere here: NotHere {
        property int c: b + 1
    }

    Component.onCompleted: doesNotExist()

    signal foo()
    signal bar()
    // Cannot assign potential undefined
    onFoo: objectName = self.bar()
}
