pragma Strict
import QtQml

QtObject {
    id: self
    property string stringArg: "a %1 thing".arg("foozly")
    property string falseArg: "a %1 thing".arg(false)
    property string trueArg: "a %1 thing".arg(true)
    property string zeroArg: "a %1 thing".arg(0)
    property string intArg: "a %1 thing".arg(11)
    property string realArg: "a %1 thing".arg(12.25)
}
