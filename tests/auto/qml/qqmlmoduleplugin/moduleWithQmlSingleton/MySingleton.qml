pragma Singleton
import QtQuick 2.0

QtObject {
    property Loader _loader: Loader {
        source: "internal/InternalType.qml"
    }

    Component.onCompleted: {
        if (tracker.objectName === "first")
            tracker.objectName = "second"
        else
            tracker.objectName = "first"
        //console.log("created singleton", this)
    }
}
