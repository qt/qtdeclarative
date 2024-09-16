import QtQml

QtObject {
    id: root
    property bool wasNotMarkedBefore: false
    property bool wasMarkedAfter: false
    property int result: -2
    function f() {
        let a = [1, 2, 3];
        function inner() {
            a = [4, 5, 6];
        }
        __forceJit(inner);
        __setupGC();
        root.wasNotMarkedBefore = !__isMarked(a);
        inner();
        root.wasMarkedAfter = __isMarked(a);
        return a[0];
    }
    Component.onCompleted: {
        if (!__forceJit(f)) {
            root.result = -1;
            return;
        }
        if (f() !== 4)
            root.result = 1;
        else if (!wasNotMarkedBefore)
            root.result = 2;
        else if (!wasMarkedAfter)
            root.result = 3;
        else
            root.result = 0; // success
    }
}
