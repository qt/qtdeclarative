pragma Strict
import QML

QtObject {
    function coerceToNumber(a: var) : real {
        return a - 1;
    }

    property real a: coerceToNumber("true")
    property real b: coerceToNumber("false")
    property real c: coerceToNumber(4)
    property real d: coerceToNumber(null)
    property real e: coerceToNumber(undefined)
    property real f: coerceToNumber("11")
}
