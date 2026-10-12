import QtQuick 2.0
import qqmllanguage.Test 1.0

MyTypeObject {
    enumProperty: MyTypeObject.EnumVal1
    Component.onCompleted: {
        var a = MyTypeObject.EnumVal1;
        var b = MyTypeObject.lowercaseEnumVal
    }
}
