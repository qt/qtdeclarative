import QtQuick

Item {
    property A a
    property list<A> aaa
    default property A myDefaultProperty

    a: A { // ok: A does not construct a B, despite containing one
        helloProperty: 3
        onHelloSignal: helloMethod()
        onHelloPropertyChanged: helloMethod()
    }
    aaa: [ A { helloProperty: 3 }] // ok: A does not construct a B, despite containing one

    property int hello
    function f(): int {
        return aaa.length
    }
}
