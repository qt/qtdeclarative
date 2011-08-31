import Qt.test 1.0
import QtQuick 2.0

QtObject {
    id: root

    property int count: 0
    signal testSignal
    onTestSignal: count++

    property int funcCount: 0
    function testFunction() {
        funcCount++;
    }

    //should increment count
    function testSignalCall() {
        testSignal()
    }

    //should NOT increment count, and should throw an exception
    property string errorString
    function testSignalHandlerCall() {
        try {
            onTestSignal()
        } catch (error) {
            errorString = error.toString();
        }
    }

    //should increment funcCount once
    function testSignalConnection() {
        testSignal.connect(testFunction)
        testSignal();
        testSignal.disconnect(testFunction)
        testSignal();
    }

    //should increment funcCount once
    function testSignalHandlerConnection() {
        onTestSignal.connect(testFunction)
        testSignal();
        onTestSignal.disconnect(testFunction)
        testSignal();
    }

    //should be defined
    property bool definedResult: false
    function testSignalDefined() {
        if (testSignal !== undefined)
            definedResult = true;
    }

    //should be defined
    property bool definedHandlerResult: false
    function testSignalHandlerDefined() {
        if (onTestSignal !== undefined)
            definedHandlerResult = true;
    }
}
