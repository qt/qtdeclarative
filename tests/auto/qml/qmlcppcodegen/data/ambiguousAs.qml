pragma Strict
import QtQml

QtObject {
    id: self
    property bool useSelf: true
    property QtObject other: {
        var a;
        if (useSelf)
            a = self
        else
            a = 15
        return a as QtObject
    }
}
