import QtQml

QtObject {
    property int i: Math.max(1, 2) // OK
    function f() { return 1 } // Error: No specified return type
    property string s: g() // Error: g?
}
