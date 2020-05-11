import QtQml 2.14
import qt.test 1.0

TestItem {
    property var vector
    property var myset
    positions: vector
    barrays: myset
    convertibles: ["hello", "world"]
}
