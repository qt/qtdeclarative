import QtQml

QtObject {
    property string a: "alpha"
    property string b: "beta"
    function f() { return a + b; }
}
