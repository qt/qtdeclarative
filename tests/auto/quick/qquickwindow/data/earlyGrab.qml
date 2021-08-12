import QtQuick
import Test

Window {
    id: window
    width: 400
    height: 400
    color: "red"

    // Important for the test to set visible early on, not wait until
    // show() from C++.  What is really verified here is that the
    // content grabbing takes the real window state into account
    // (visible vs. exposed).
    visible: true

    Grabber {
        id: grabber
        objectName: "grabber"
    }

    Item {
        anchors.fill: parent
        Component.onCompleted: grabber.grab(window)
    }
}
