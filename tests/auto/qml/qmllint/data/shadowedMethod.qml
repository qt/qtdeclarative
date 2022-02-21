import QtQuick

QtObject {
    function foo() {}
    property bool foo: false

    Component.onCompleted: foo()
}
