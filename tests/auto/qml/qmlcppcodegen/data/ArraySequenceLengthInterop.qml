import QtQml
import TestTypes

QtObject {
    property var seq:  SequenceTypeExample {
        id: mySequence
    }

    property int length: mySequence.qrealListProperty.length

    function goodSequenceWrite() {
        var someData = [1.1, 2.2, 3.3]
        someData.length = 100;
        for (var i = 0; i < 5; ++i) {
            for (var j = 0; j < 100; ++j) {
                someData[j] = j;
            }
            //Modifies entire sequence at once by copying a temp variable
            mySequence.qrealListProperty = someData;
        }
    }

    Component.onCompleted: goodSequenceWrite()
}
