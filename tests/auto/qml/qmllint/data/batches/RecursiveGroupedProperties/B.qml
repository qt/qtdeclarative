import QtQuick

Item {
    property A a
    property list<A> aaa

    a.hello: 123

    property int hello
    function f(): int {
        return aaa.length
    }
}
