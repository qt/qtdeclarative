pragma Strict
import QtQml

QtObject {
    property Person inner: Person {
        function getName() : int { return 5 }
    }

    property Person none: Person {}

    property Person evil: Person {
        property string getName: "not a function"
    }

    onObjectNameChanged: console.log(inner.getName())

    function swapNone() {
        let t = inner;
        inner = none;
        none = t;
    }

    function swapEvil() {
        let t = inner;
        inner = evil;
        evil = t;
    }
}
