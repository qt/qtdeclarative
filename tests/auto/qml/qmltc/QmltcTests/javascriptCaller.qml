import QtQml
import "subfolder/code.js" as TheCode

QtObject {
    property bool valueIsBad: TheCode.isGood("3")
    property bool valueIsGood: TheCode.isGood(".")
}
