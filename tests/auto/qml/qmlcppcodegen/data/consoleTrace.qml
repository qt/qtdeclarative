import QtQml

QtObject {
    function a() { b() }
    function b() { c() }
    function c() { console.trace() }
    Component.onCompleted: a()
}
