import QtQml 2.12

QtObject {
    id: root
    property bool test1: false;
    property bool test2: false;
    property bool test3: false;
    property bool done: false;
    function *gen() {
        yield 1
        yield 2
        yield 3
    }

    Component.onCompleted: {
        let it = root.gen();
        root.test1 = (it.next().value == 1);
        root.test2 = (it.next().value == 2);
        root.test3 = (it.next().value == 3);
        root.done = it.next().done;
    }
}
