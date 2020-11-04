import QtQml 2.0
import qt.test 1.0

QtObject {
    Component.onCompleted: {
        var myList = SequenceConvertObject.getValues()
        SequenceConvertObject.call(myList)
    }
}

