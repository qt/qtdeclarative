import QtQuick

Item {
    width: 200; height: 200

    Rectangle {
        width: 100; height: 100; x: 0; y: 100
        color: "cyan"
    }

    Rectangle {
        width: 100; height: 100; x: 100; y: 100
        color: "green"

        TapHandler {}
    }
}
