import QtQuick 2.15

Item {
    id: root

    property alias symbol: symbol
    symbol.layer.enabled: true

    Item {
        id: symbol
    }

    Rectangle {
        id: txtElevationValue

        property Rectangle background: Rectangle { }

        state: "ValidatorInvalid"

        states: [
            State {
                name: "ValidatorInvalid"
                PropertyChanges {
                    target: txtElevationValue
                    background.border.color: "red" // this line caused the segfault in qtbug107795
                }
            },
            State {
                name: "ValidatorAcceptable"
            }
        ]
    }
}
