pragma FunctionSignatureBehavior: Ignored
import StaticTest
import QtQml
QtObject {
    property rect rect: ({ x: 12, y: 13 })
    property withLength withLength: 5

    function a(r: rect) {
        r.x = 77 // writes back
    }

    function b(s: string) : int {
        return s.length // this is not in fact a string
    }

    function c(w: withLength) : int {
        return w.length || 67
    }

    property int l: b(withLength)
    property int m: rect.x
    property int n: c(99) // passes a number

    Component.onCompleted: a(rect)
}
