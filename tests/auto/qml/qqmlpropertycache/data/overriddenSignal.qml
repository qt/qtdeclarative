import QtQml
import Test.PropertyCache

QtObject {
    id: root

    property BaseObject obj: null
    property Connections connection: Connections {
        target: obj
        function onPropertyAChanged() { ++root.a }
    }
    onObjChanged: {
        connection.target = obj // Make sure this takes effect before sending the signal
        obj.propertyAChanged()
    }

    property BaseObject obj2: null
    property Connections connection2: Connections {
        target: obj2
        function onSignalA() { ++root.b }
    }
    onObj2Changed: {
        connection2.target = obj2 // Make sure this takes effect before sending the signal
        obj2.signalA();
    }

    property BaseObject theObj: BaseObject {}
    Component.onCompleted: {
        // Make sure the change signals are triggered also initially
        obj = theObj;
        obj2 = theObj;
    }

    property int a: 0
    property int b: 0
}
