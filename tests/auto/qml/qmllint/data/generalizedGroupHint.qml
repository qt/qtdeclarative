import QtQuick

Window {
    width: 640
    height: 480
    visible: true
    title: 'Resolve my color type'

    Item {
        id: foo

        states: [
            State {
                PropertyChanges {
                    target: foo
                    myColor: Qt.rgba(0.5, 0.5, 0.5, 0.16)
                }
            }
         ]

        property color myColor: 'black'
    }
}
