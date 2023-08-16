import QtQuick
import TestTypes

BirthdayParty {
    id: party
    host: Person {
        name: "Bob Jones"
        shoeSize: 12
    }
    guests: [
        Person { name: "Leo Hodges" },
        Person { name: "Jack Smith" },
        Person { name: "Anne Brown" }
    ]

    property var dresses: [0, 0, 0, 0]

    property var foo: party
    property var bar: console

    property var numeric
    property var stringly

    Component.onCompleted: {
        invite("William Green")
        foo.invite("The Foo")
        bar.log("The Bar")
        numeric = 1;
        stringly = "name";
    }

    function storeElement() {
        ++host.shoeSize
        party.dresses[2] = [1, 2, 3]
    }

    function stuff(sn) {
        // Warning: to not test for signal handlers like this in actual code.
        // Use the helper methods in QQmlSignalNames instead.
        if (sn.substr(0, 2) === "on" && sn[2] === sn[2].toUpperCase())
            return sn
        return "on" + sn.substr(0, 1).toUpperCase() + sn.substr(1)
    }

    function retrieveVar() {
        return guests[numeric][stringly];
    }

    function retrieveString() : string {
        return guests[numeric][stringly];
    }

    property var n1: stuff("onGurk")
    property var n2: stuff("semmeln")
    property var n3: stuff(12)

    property int enumValue: Item.TopRight
}
