import QtQuick 2.0

Rectangle {
    id: root
    width: 100; height: 100

    states: [
        State {
            name: "red_color"
            PropertyChanges { root.color: "red" }
        },
        State {
            name: "blue_color"
            PropertyChanges { root.color: "blue" }
        }
    ]
}
