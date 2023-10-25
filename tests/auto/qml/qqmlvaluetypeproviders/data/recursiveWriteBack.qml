import QtQml
import Test

MyTypeObject {
    property list<structured> l: [{i : 21}, {c: 22}, {p: {x: 199, y: 222}}]
    property int aa: 5

    Component.onCompleted: {
        l[2].i = 4
        l[1].p.x = 88
        l[0].sizes[1].width = 19
        structured.p.x = 76

        var sizesDetached = l[0].sizesDetached();
        sizesDetached[1].width = 12;
        aa = sizesDetached[1].width;
    }
}
