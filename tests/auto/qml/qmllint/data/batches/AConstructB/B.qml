import QtQuick

Item {
    property A a
    property list<A> aaa
    default property A myDefaultProperty

    property int helloProperty
    function helloMethod() { return aaa.length; }
    signal helloSignal
}
