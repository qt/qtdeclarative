import QtQuick 2.0
import Qt.test 1.0 as ModApi

Item {
    id: root

    property bool success: false
    property bool c1HasBeenDestroyed: false

    property Item c1 // not a js reference, so won't keep it alive

    SignalEmittedComponent {
        id: c2
        property int c1a: if (root.c1) root.c1.a; else 0; // will change during onDestruction handler of c1.
        function c1aChangedHandler() {
            // this should still be called, after c1 has been destroyed by gc,
            // because the onDestruction handler of c1 will be triggered prior
            // to when c1 will be invalidated.
            if (root.c1HasBeenDestroyed && c1a == 20) root.c1.setSuccessPropertyOf(root, true);
        }
    }

    Component.onCompleted: {
        // dynamically construct sibling.  When it goes out of scope, it should be gc'd.
        // note that the gc() will call weakqobjectcallback which will set queued for
        // deletion flag -- thus QQmlData::wasDeleted() will return true for that object..
        var c = Qt.createComponent("SignalEmittedComponent.qml", root);
        var o = c.createObject(null); // JS ownership
        o.onAChanged.connect(c2.c1aChangedHandler);
        c1 = o;
        c1HasBeenDestroyed = true;
        // return to event loop.
    }

    function destroyC2() {
        // we must gc() c1 first, then destroy c2, then handle events.
        c2.destroy();
    }
}
