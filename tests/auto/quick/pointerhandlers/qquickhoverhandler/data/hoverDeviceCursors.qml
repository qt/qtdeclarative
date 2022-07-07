import QtQuick

Item {
    width: 200; height: 200

    HoverHandler {
        objectName: "stylus"
        acceptedDevices: PointerDevice.Stylus
        acceptedPointerTypes: PointerDevice.Pen
        cursorShape: Qt.CrossCursor
    }

    HoverHandler {
        objectName: "stylus eraser"
        acceptedDevices: PointerDevice.Stylus
        acceptedPointerTypes: PointerDevice.Eraser
        cursorShape: Qt.PointingHandCursor
    }

    HoverHandler {
        objectName: "airbrush"
        acceptedDevices: PointerDevice.Airbrush
        acceptedPointerTypes: PointerDevice.Pen
        cursorShape: Qt.BusyCursor
    }

    HoverHandler {
        objectName: "airbrush eraser"
        acceptedDevices: PointerDevice.Airbrush
        acceptedPointerTypes: PointerDevice.Eraser
        cursorShape: Qt.OpenHandCursor
    }

    HoverHandler {
        objectName: "mouse"
        acceptedDevices: PointerDevice.Mouse
        // acceptedPointerTypes can be omitted because Mouse is not ambiguous.
        // When a genuine mouse move is sent, there's a conflict, and this one should win.
        cursorShape: Qt.IBeamCursor
    }

    HoverHandler {
        objectName: "conflictingMouse"
        acceptedDevices: PointerDevice.Mouse
        // acceptedPointerTypes can be omitted because Mouse is not ambiguous.
        // When a genuine mouse move is sent, there's a conflict, and this one should lose.
        cursorShape: Qt.ClosedHandCursor
    }
}
