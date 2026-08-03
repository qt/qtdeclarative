import QtQuick

Item {
    property list<B> bbb

    property int hello

    function f(): int {
        return bbb[0].hello
    }
}
