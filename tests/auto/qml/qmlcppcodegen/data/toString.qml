import QtQml

QtObject {
    id: self
    property QtObject other: QtObject {
        function toString() : string { throw "no" }
    }

    function toString() : string { return "yes" }

    property string yes: self + " yes"
    property string no: other + " no"
}
