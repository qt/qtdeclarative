import QtQuick 2.0

Item {
    Component.onCompleted: {
        var testVar = variantObject.getIndex()
        variantObject.selection(testVar, 1)
    }
}

