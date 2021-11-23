pragma Strict
import QtQml

QtObject {
    function foo(a: int) : int { return a + 10 }
    property int bar: foo(10 + 10)
}
