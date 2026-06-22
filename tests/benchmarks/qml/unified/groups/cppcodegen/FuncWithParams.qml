pragma Strict
import QtQml

QtObject {
    id: self
    property int a: 0
    function foo(p: int) : int { return a + p }
    property int b: foo(10)

    property int c: 0
    function bar(p: int) : int { return c + p }
    property int d: self.bar(10)
}
