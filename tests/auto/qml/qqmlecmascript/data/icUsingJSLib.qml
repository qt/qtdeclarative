import QtQml

import "Lib.js" as Lib

QtObject {
    component C : QtObject {
        property int num: Lib.f()
    }

    property var test: C { id: c }
    property alias num: c.num
}
