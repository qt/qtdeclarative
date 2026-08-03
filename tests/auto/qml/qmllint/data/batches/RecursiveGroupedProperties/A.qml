import QtQuick

Item {
    property B b
    property list<B> bbb

    b.hello: 12345

    property int hello

    function f(): int {
        return bbb.length
    }
}
