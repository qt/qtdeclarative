import QtQml 2.0

QtObject {
    id: root

    property bool success: false

    signal testSignal(param: bool)

    function handleTestSignal(param) {
        success = param
    }

    Component.onCompleted: {
        success = false;
        root.testSignal.connect(handleTestSignal)
        root.testSignal(true);
    }
}
