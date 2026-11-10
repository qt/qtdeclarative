import QtQml
import TestTypes

QtObject {
    readonly property Component withName: QtObject {
        property int foo
        function getFoo(): int { return foo }
    }
    readonly property Component withoutName: QtObject {}

    readonly property ScriptStringHolder load: ScriptStringHolder { ss: foo }
    readonly property ScriptStringHolder call: ScriptStringHolder { ss: getFoo() }
}
