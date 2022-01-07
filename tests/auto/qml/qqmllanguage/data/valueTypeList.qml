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

    Component.onCompleted: {
        a[2] = 17
        y.increment()
    }

    onObjectNameChanged: {
        x[1].increment()
        b[1].x = 12
    }
}
