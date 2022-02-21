import QtQml

QtObject {
    function foo() {}
    Component.onCompleted: Qt.callLater(foo);
}
