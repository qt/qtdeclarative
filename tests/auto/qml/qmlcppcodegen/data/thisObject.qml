pragma Strict
import QtQml

QtObject {
    property QtObject warned

    function f(arg: QtObject) { warned = arg }
    function warn() { f(this) }

    Component.onCompleted: warn()
}
