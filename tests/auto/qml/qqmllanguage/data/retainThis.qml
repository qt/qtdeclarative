import QmlOtherThis
import QtQml

QtObject {
    property var cppMethod: objA.greet
    property var cppMethod2: objA.sum

    property Greeter a: Greeter { id: objA; objectName: "objA" }
    property Greeter b: Greeter { id: objB; objectName: "objB" }

    function doCall() {
        cppMethod.call(objB)
        cppMethod2(5, 6)
    }

    property var cppMethod3;
    function doRetrieve(g) {
        cppMethod3 = g.greet;
    }

    function doCall2() {
        cppMethod3();
    }

    property Greeter c: Greeter {
        id: objC
        objectName: "objC"

        property var cppMethod: objC.sum

        function doCall() {
            cppMethod(7, 7)
        }
    }

    Component.onCompleted: {
        doCall();
        doCall();

        doRetrieve(objA);
        doCall2();
        doRetrieve(objB);
        doCall2();

        objC.doCall();
        objC.cppMethod = objB.sum;
        objC.doCall();
    }
}
