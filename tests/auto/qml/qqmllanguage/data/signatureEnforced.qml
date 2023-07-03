pragma FunctionSignatureBehavior: Enforced

import StaticTest
import QtQml

QtObject {
    property rect rect: ({ x: 12, y: 13 })
    property withLength withLength: 5

    function a(r: rect) {
        r.x = 77 // does write back, but this is an evil thing to do.
    }

    function b(s: string) : int {
        return s.length // this is a string
    }

    function c(w: withLength) : int {
        return w.length || 67;
    }

    function d(r) {
        r.y = 77 // does write back
    }

    property int l: b(withLength)
    property int m: rect.x
    property int n: c(99) // creates a withLength
    property int o: rect.y

    function bad(b: int) { return b }

    Component.onCompleted: {
        a(rect)
        d(rect)
        bad(15)
    }
}
