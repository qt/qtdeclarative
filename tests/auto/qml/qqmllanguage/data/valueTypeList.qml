import QtQml
import ValueTypes

ValueTypeListBase {
    a: [0, 2, 3, 1]
    b: [Qt.point(1, 2), Qt.point(3, 4)]
    property int c: a[2]
    property point d: b[1]
    property derived y
    x: [y, y, y]
    baseList: [y, y, y]
    property list<rect> e: [Qt.rect(1,1,1,1), Qt.rect(2,2,2,2), Qt.rect(3,3,3,3)]
    property rect f: e[0]

    Component.onCompleted: {
        a[2] = 17
        y.increment()
        e[0] = Qt.rect(a[0],a[1],a[2],a[3])
    }

    onObjectNameChanged: {
        x[1].increment()
        b[1].x = 12
    }
}
