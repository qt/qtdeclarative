import Qt.test 1.0

MyQmlObject {
    property int argumentCount: -1
    property bool calleeCorrect: false
    onBasicSignal: {
        argumentCount = arguments.length
        calleeCorrect = (arguments.callee === onBasicSignal)
        setString('pass')
    }
}
