import QtQuick

Window {
    Item {
        id: foo
        property color myColor: 'black'

        states: [
            State {
                PropertyChanges {
                    target: foo
                    myColor: Qt.rgba(0.5, 0.5, 0.5, 0.16)
                    notThere: "a"
                }
            }
         ]

    }
}
