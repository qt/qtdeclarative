import QtQuick

Item {
    function f() { eval(); }
    function g() { eval("1 + 1"); }
}
