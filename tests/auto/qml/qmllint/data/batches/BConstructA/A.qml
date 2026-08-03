import QtQuick

Item {
    property B b
    property list<B> bbb
    default property B myDefaultProperty

    property int helloProperty
    function helloMethod() { return bbb.length; }
    signal helloSignal
}
