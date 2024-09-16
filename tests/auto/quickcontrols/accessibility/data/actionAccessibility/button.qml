import QtQuick
import QtQuick.Controls

Button {
    action: Action {
        id: anAction
        text: "Peaches"
        Accessible.name: "Peach"
        Accessible.description: "Show peaches some love"
    }
    text: Accessible.description
}
