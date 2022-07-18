import QtQml
import JsLibrary as JLib
import "JsLibrary/HelperLibrary.js" as HelperLib

QtObject {
    property string text1: JLib.HelperLibrary.foo()
    property string text2: HelperLib.foo()
}
