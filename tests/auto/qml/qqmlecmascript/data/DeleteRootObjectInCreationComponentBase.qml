import QtQuick 2.0
import Qt.test.qobjectApi 1.0 as ModApi

Rectangle {
    id: base
    color: "red"

    function flipOwnership() {
        ModApi.trackObject(base);
        ModApi.trackedObject(); // flip the ownership.
        if (!ModApi.trackedObjectHasJsOwnership())
            derived.testConditionsMet = false;
        else
            derived.testConditionsMet = true;
    }

    onColorChanged: {
        // will be triggered during beginCreate of derived
        flipOwnership();
        gc();
        gc();
        gc();
    }
}
