import Qt.test 1.0

MyQmlObject {
    property int argumentCount: -1
    property bool calleeCorrect: false

    onArgumentSignal: {
        argumentCount = arguments.length
        calleeCorrect = (arguments.callee === onArgumentSignal)
        setString('pass ' + arguments[0] + ' ' + arguments[1] + ' '
                          + arguments[2] + ' ' + arguments[3] + ' '
                          + arguments[4])
    }
}
