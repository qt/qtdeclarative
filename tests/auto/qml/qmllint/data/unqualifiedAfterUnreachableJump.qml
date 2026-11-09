import QtQml

QtObject {
    function f(x: int): int {
        if (x)
            return 1
        else
            return unknownThing
    }
}
