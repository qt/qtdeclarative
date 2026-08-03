import QtQuick

Item {
    property A a
    property list<A> aaa
    default property A myDefaultProperty

    a: A {
        hello: 3
    }
    aaa: [ A { hello: 3 }] // note: A constructs B

    property int hello
    function f(): int {
        return aaa.length
    }
}
