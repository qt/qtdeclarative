pragma ValueTypeBehavior: Addressable
import QtQml

QtObject {
    id: root
    property rect r: Qt.rect(10, 20, 3, 4)
    property var v: r
    property real x: (v as rect).x

    function f(input: bool) : var {
        if (input)
            return 0
        return Qt.point(2, 2)
    }

    property var vv: Qt.point(5, 5)
    property var uu: undefined

    property int tv3: (root.vv as point)?.x
    property var tv4: (root.uu as rect)?.x
    property int tc3: (root?.vv as point)?.y
    property var tc6: (root?.uu as rect)?.height
    property var tc7: (f(true) as point)?.x
    property var tc8: (f(false) as point)?.x
}
