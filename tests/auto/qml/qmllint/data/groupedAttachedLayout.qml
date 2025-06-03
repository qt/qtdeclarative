import QtQuick
import QtQuick.Layouts

Window {
    id: root

    Rectangle {
        id: redRect
    }

    Item {
        states: [
            State {
                PropertyChanges {
                    redRect.Layout.fillWidth: true
                }
            }
        ]
    }
}
