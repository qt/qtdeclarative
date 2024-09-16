import QtQuick

Item {
    function f(a,b,c) {
        a == b ? b == c ? c : b + 3 : 42
    }
}
