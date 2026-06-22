pragma Strict
import QtQml

QtObject {
    property int a: 10
    property int b: 20
    property int c: 30

    property list<int> l: [a, b, c, 45]
    property int o: l[0]
}
