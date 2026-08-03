import QtQuick

Item {
    property list<A> aaa

    property int hello

    aaa: [ A { hello: 123; } ]
}
