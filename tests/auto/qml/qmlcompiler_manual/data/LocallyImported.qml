import QtQml
QtObject {
    id: foo
    property int count: 0
    function getMagicValue() {
        var c = foo.count;
        foo.count++;
        return foo.count + (c * 2);
    }

    Component.onCompleted: {
        ++count;
    }
}
