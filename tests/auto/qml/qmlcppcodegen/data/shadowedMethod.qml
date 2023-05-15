pragma Strict
import QtQuick

Item {
    component B: Item {
        function contains(point: point) : string {
            return "b"
        }
    }


    component C: Item {
        function contains(point: point) : string {
            return "c"
        }
    }

    property Item a: Item {}
    property B b: B {}
    property C c: C {}

    function doThing() : var { return a.contains(Qt.point(0, 0)) }

    property var athing;
    property var bthing;
    property var cthing;

    Component.onCompleted: {
        athing = doThing();
        a = b;
        bthing = doThing();
        a = c;
        cthing = doThing();
    }
}
