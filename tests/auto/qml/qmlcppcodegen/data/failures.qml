import QtQml
import TestTypes

QtObject {
    property string attachedForNonObject: objectName.Component.objectName
    property string attachedForNasty: Nasty.objectName

    property Nasty nasty: Nasty {
        objectName: Component.objectName
    }

    onFooBar: console.log("nope")

    function doesReturnValue() { return 5; }
}
