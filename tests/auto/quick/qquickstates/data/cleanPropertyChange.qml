import QtQuick

Rectangle {
    id: extendedRect
    property color extendedColor: "cyan"
    signal didSomething()

    width: 100
    height: 100
    color: "red"

    states: State {
        name: "green"
        PropertyChanges {
            extendedRect {
                color: "yellow"
                width: 90
                height: 90
                extendedColor: "blue"
                onDidSomething: {
                    extendedRect.color = "green"
                    extendedRect.extendedColor = "green"
                }
            }
        }
    }
}
