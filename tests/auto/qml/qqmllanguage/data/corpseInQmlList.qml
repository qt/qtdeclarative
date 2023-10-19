import QtQml

QtObject {
    property var b;

    function returnList(a: QtObject) : list<QtObject> {
        return [a]
    }

    function setB(a: QtObject) {
        b = { b: returnList(a) }
    }
}
