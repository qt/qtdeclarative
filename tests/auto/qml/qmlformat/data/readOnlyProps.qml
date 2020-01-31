import QtQuick 2.0

QtObject {
    // Testing UiObjectBinding
    readonly property Item item: Item { id: test; signal foo() }
    // End comment

    // Testing UiArrayBinding
    readonly property list<Item> array: [ Item { id: test1; signal foo() }, Item { id: test2; signal bar() } ]

    // Testing UiScriptBinding
    readonly property int script: Math.sin(Math.PI)

    property bool normalProperty: true
}
