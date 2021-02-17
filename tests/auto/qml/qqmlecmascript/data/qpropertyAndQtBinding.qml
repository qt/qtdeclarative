import QtQuick 2.6
import test

Tester {
    id: tester
    property var num: 100
    property int i: 42


    Component.onCompleted: {
        function s(x) {
            return x
        }
        function c(x) {
            return x + num
        }

        try {
            tester.readOnlyBindable = Qt.binding(() => tester.i)
        } catch (e) {
            console.warn(e)
        }
        tester.nonBound = Qt.binding(() => tester.i)
        let bound = s.bind(undefined, 100)
        tester.simple = Qt.binding(bound)
        bound = c.bind(undefined, 100)
        tester.complex = Qt.binding(bound)
    }
}
