import QtQml
import Test.PropertyCache

QtObject {
    property BaseObject obj: BaseObject {}
    property int a: obj.finalProp

    property BaseObject obj2: BaseObject {
        property int callCount: 0
        function finalProp() { ++callCount }
        Component.onCompleted: finalProp()
    }

    property var b: obj2.finalProp
    Component.onCompleted: obj2.finalProp()

    property int c: obj2.callCount
}
