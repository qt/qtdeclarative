import QtQml
import TestTypes

QtObject {
    readonly property QtObject named: QtObject { id: foo }
    readonly property ScriptStringHolder load: ScriptStringHolder { ss: foo }
}
