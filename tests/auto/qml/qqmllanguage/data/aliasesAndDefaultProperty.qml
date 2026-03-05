import QtQuick

Foo {

    component Foo: Item {
        id: root

        property alias label: root.text
        default property alias foo: it.data

        Item {
            id: it
        }
        property string text: "should not change"
    }
}
