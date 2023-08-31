import QtQml

QtObject {

    function patron(a, ...b) {
    }

    function patron1(a, ...[b, ...args]) {
    }

    function patron2(...{}) {
    }
}
