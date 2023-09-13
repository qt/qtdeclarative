import QtQuick 2.15

Item {
    id: theFoo

    required data
    required property Item theItem

    function foo() {
        theItem.foo("The issue is exacerbated if the object literal is wrapped onto the next line like so:", {
            "foo": theFoo
        });
    }
}
