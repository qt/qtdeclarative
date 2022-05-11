import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    spacing: 2

    height: 100

    ComboBox {
        model: ["1", "2", "3", "4", "5", "6"]
    }

    ComboBox {
        editable: true
        focus: true
        popup.visible: true
        model: ListModel {
            ListElement { text: "Coca Cola" }
            ListElement { text: "Fanta" }
            ListElement { text: "Sprite" }
        }
    }

    ComboBox {
        editable: true
        model: ListModel {
            ListElement { text: "Coca Cola" }
            ListElement { text: "Fanta" }
            ListElement { text: "Sprite" }
        }
    }

    ComboBox {
        down: true
        model: ListModel {
            ListElement { text: "Coca Cola" }
            ListElement { text: "Fanta" }
            ListElement { text: "Sprite" }
        }
    }

    ComboBox {
        flat: true
        selectTextByMouse: true
        enabled: false
        model: ListModel {
            ListElement { text: "Ski" }
            ListElement { text: "Snowboard" }
            ListElement { text: "Cross-country" }
        }
    }
}
