import QtQuick

Item {
    function f(a,b,c) {
        f(a,a,a), b += 55,c *= 24;
    }
}
