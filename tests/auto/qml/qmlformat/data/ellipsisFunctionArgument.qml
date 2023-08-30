import QtQml

QtObject {

    function patron(a, ...b) {
        const x = "x"
    }

    function patron1(a, ...[b, ...args]) {
        const x = "x"
    }

    function patron2(...{}) {
        const x = "x"
    }
}