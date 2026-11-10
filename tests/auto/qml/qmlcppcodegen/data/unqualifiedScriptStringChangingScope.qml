import QtQml
import TestTypes

ScriptStringHolder {
    id: root
    property int foo: 1
    function getFoo(): int { return foo }

    readonly property QtObject withFoo: QtObject {
        property int foo: 2
        function getFoo(): int { return foo }
    }
    readonly property QtObject withoutFoo: QtObject {}
    readonly property QtObject outer: QtObject {
        property int foo: 3
        function getFoo(): int { return foo }
    }

    readonly property ScriptStringHolder load: ScriptStringHolder { ss: foo }
    readonly property ScriptStringHolder call: ScriptStringHolder { ss: getFoo() }
}
