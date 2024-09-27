pragma Strict
import QtQml

QtObject {
    property Dummy d: Dummy {}

    function returnDummy() : Dummy {
        return d;
    }

    function returnDummy2() : Dummy2 {
        return d.child2;
    }

    property Dummy d2: returnDummy()
    property QtObject d3: returnDummy2();
}
