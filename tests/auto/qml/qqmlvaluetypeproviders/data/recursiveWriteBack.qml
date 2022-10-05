import QtQml
import Test

MyTypeObject {
    property list<structured> l: [{i : 21}, {c: 22}, {p: {x: 199, y: 222}}]

    Component.onCompleted: {
        l[2].i = 4
        l[1].p.x = 88
        l[0].sizes[1].width = 19
        structured.p.x = 76
    }
}
