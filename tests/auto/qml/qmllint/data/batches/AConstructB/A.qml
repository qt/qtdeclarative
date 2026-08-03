import QtQuick

Item {
    property B b
    property list<B> bbb
    default property B myDefaultProperty

    b: B { helloProperty: 42; onHelloSignal: helloMethod(); onHelloPropertyChanged: helloMethod(); }
}
