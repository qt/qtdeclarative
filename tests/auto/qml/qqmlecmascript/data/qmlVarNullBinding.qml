import QtQml 2.2

QtObject {
    property var foo: null
    property bool signalSeen: false
    onFooChanged: signalSeen = true
}
