import QtQuick
import QtQuick.Controls

Window {
    id: window

    Text {text: "Hello"}
    Popup {
        id: popup
        anchors.centerIn: parent
        dim: true
        property double dimmerOpacity: 0.5

        Overlay.modeless: Rectangle {
            color: "blue"
            opacity: popup.dimmerOpacity

            Behavior on opacity {
                NumberAnimation { from: 0; to: popup.dimmerOpacity; duration: 250 }
            }
        }

        Overlay.modal: Rectangle {
            color: "blue"
            opacity: popup.dimmerOpacity

            Behavior on opacity {
                NumberAnimation { from: 0; to: popup.dimmerOpacity; duration: 250 }
            }
        }

        enter: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 250 }
        }
        exit: Transition {
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 250 }
        }
    }
}
