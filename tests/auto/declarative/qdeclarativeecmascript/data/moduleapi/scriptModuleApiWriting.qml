import QtQuick 2.0
import Qt.test.scriptApi 1.0 as QtTestScriptApi
import Qt.test.scriptApi 2.0 as QtTestScriptApi2

QtObject {
    property int firstProperty
    property int readBack

    property int secondProperty
    property int unchanged

    onFirstPropertyChanged: {
        if (QtTestScriptApi.scriptTestProperty != firstProperty) {
            QtTestScriptApi.scriptTestProperty = firstProperty;
            readBack = QtTestScriptApi.scriptTestProperty;
        }
    }

    onSecondPropertyChanged: {
        if (QtTestScriptApi2.scriptTestProperty != secondProperty) {
            QtTestScriptApi2.scriptTestProperty = secondProperty;
            unchanged = QtTestScriptApi2.scriptTestProperty;
        }
    }

    Component.onCompleted: {
        firstProperty = QtTestScriptApi.scriptTestProperty;
        readBack = QtTestScriptApi.scriptTestProperty;
        secondProperty = QtTestScriptApi2.scriptTestProperty;
        unchanged = QtTestScriptApi2.scriptTestProperty;
    }
}
