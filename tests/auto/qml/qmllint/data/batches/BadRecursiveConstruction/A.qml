import QtQuick

Item {
    property B b
    property list<B> bbb
    default property B myDefaultProperty

    b: B { hello: 42 } // note: B constructs A

    property int hello

    function f(): int {
        return bbb.length
    }
}
