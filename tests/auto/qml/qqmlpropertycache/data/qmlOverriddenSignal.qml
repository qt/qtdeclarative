import QtQml
import Test.PropertyCache

BaseObject {
    property int callCount: 0
    function signalA() { ++callCount }
    Component.onCompleted: signalA()
}
