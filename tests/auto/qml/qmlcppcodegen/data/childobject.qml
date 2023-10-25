import QtQml
import TestTypes

QtObject {
    property ObjectWithMethod child: ObjectWithMethod {
        objectName: "kraut"

        function doString() { overloaded("string"); }
        function doNumber() { overloaded(5.2); }
        function doArray() { overloaded({a: 2, b: 3, c: 3}); }
        function doFoo() { foo(this); }
    }
    objectName: child.objectName
    property int doneThing: child.doThing()
    property int usingFinal: child.fff

    function setChildObjectName(name: string) {
        child.objectName = name
    }
}
