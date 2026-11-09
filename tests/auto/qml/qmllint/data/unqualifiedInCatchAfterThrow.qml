import QtQml

QtObject {
    function f(x: int): int {
        try {
            throw x
        } catch (e) {
            return unknownThing
        }
    }
}
