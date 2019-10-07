pragma Singleton
import QtQuick 2.0
import Test 1.0

QtObject {
    property Loader _loader: Loader {
        source: "internal/InternalType.qml"
    }

    Component.onCompleted: {
        if (Tracker.objectName === "first")
            Tracker.objectName = "second"
        else
            Tracker.objectName = "first"
        //console.log("created singleton", this)
    }
}
