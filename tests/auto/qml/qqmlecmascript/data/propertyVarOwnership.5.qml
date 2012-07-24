import QtQuick 2.0
import Qt.test 1.0 as SingletonType

Item {
    id: testOwnership
    property bool test: false

    function runTest() {
        var o;
        var c = Qt.createComponent("ComponentWithVarProp.qml");
        if (c.status == Component.Ready) {
            o = c.createObject();
        } else {
            return; // failed to create component.
        }
        o.varprop = true;                // causes initialization of varProperties.
        SingletonType.QObject.trackObject(o);        // stores QObject ptr
        if (SingletonType.QObject.trackedObject() == null) return;        // is still valid, should have a valid v8object.
        o = new Date();                  // causes object to be gc-able.
        gc();  // collect object's v8object + varProperties, queues deleteLater.
        if (SingletonType.QObject.trackedObject() != null) return;        // v8object was previously collected.
        SingletonType.QObject.setTrackedObjectProperty("varprop");        // deferences varProperties of object.
        test = !(SingletonType.QObject.trackedObjectProperty("varprop")); // deferences varProperties of object.
        // if we didn't crash, success.
    }
}


