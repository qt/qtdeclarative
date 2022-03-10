import QtQml

QtObject {
    function foo() { return "a" + 99 }
    property var bar: foo
}
