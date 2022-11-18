pragma Strict
import QtQml

QtObject {
    function selectSecondInt(a: list<int>): int {
        return a[1]
    }

    function returnInts1(): list<int> {
        return l
    }

    function returnInts2(): list<int> {
        return [1, 2, 3, 4]
    }

    function selectSecondDummy(a: list<Dummy>): Dummy {
        return a[1]
    }

    property list<int> l: [5, 4, 3, 2, 1]
    property int i: selectSecondInt(l)
    property int j: selectSecondInt([1, 2, 3, 4, 5])
    property int i1: returnInts1()[3]
    property int i2: returnInts2()[3]

    property list<Dummy> dummies: [
        Dummy{ objectName: "a"},
        Dummy{ objectName: "this one" },
        Dummy{ }
    ]
    property Dummy d: selectSecondDummy(dummies)
}
