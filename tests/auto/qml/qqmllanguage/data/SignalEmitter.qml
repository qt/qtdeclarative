import QtQml 2.0

QtObject {
    // these two need to be set by the test qml
    property QtObject testObject
    property bool handleSignal

    property SignalParam p: SignalParam { }
    property OtherSignalParam op: OtherSignalParam { }
    signal testSignal(SignalParam spp);

    function emitTestSignal() {
        var caught = false;
        try {
            testObject.expectNull = true;
            testSignal(op);
        } catch(e) {
            // good: We want a type error here
            caught = true;
            if (handleSignal)
                testObject.determineSuccess(null);
        }
        if (!caught && handleSignal)
            testObject.determineSuccess("fail");

        testObject.expectNull = false;
        testSignal(p);
    }

    onTestSignal: {
        if (handleSignal == true) {
            testObject.determineSuccess(spp);
        }
    }
}
