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

    property string greeting1
    property string greeting2

    readonly property string defaultGreeting: "Default Greeting"
    property QtObject o: QtObject {
        id: o
        property var customGreeting
        function greet() : string {
            return (o.customGreeting as string) ?? root.defaultGreeting
        }
    }

    Component.onCompleted: {
        root.greeting1 = o.greet()
        o.customGreeting = "Custom Greeting"
        root.greeting2 = o.greet()
    }
}
