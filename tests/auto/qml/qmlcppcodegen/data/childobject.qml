import QtQml
import TestTypes

QtObject {
    property ObjectWithMethod child: ObjectWithMethod {
        objectName: "kraut"
    }
    objectName: child.objectName
    property int doneThing: child.doThing()
    property int usingFinal: child.fff

    function setChildObjectName(name: string) {
        child.objectName = name
    }
}
