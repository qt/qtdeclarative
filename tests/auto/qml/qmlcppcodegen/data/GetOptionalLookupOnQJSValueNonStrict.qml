import QtQml
import TestTypes

QtObject {
    property Action action: Action { }
    property bool b: action?.visible
}
