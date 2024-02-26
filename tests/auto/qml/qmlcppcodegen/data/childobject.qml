import QtQml
import TestTypes

QtObject {
    property ObjectWithMethod child: ObjectWithMethod {
        objectName: "kraut"

        function doString() { overloaded("string"); }
        function doNumber() { overloaded(5.2); }
        function doArray() { overloaded({a: 2, b: 3, c: 3}); }

        function doString2() { overloaded2("string"); }
        function doNumber2() { overloaded2(5.2); }

        // Artificially pass an extra argument to avoid choosing the "string" overload.
        // Unfortunately this is still order-dependent on the metaobject level.
        function doArray2() { overloaded2({a: 2, b: 3, c: 3}, 1); }

        function doFoo() { foo(this); }
    }
    objectName: child.objectName
    property int doneThing: child.doThing()
    property int usingFinal: child.fff

    function setChildObjectName(name: string) {
        child.objectName = name
    }
}
