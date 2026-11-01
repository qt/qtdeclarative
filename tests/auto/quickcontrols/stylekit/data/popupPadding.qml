import QtQuick
import Qt.labs.StyleKit

Item {
    width: 300
    height: 400

    property alias comboBox: comboBox
    property alias searchField: searchField

    // Popups with padding: their lists must still have room for all items
    StyleKit.style: Style {
        themeName: "Light"
        popup.padding: 10
    }

    ComboBox {
        id: comboBox
        x: 10; y: 10
        model: ["One", "Two", "Three"]
    }

    SearchField {
        id: searchField
        x: 10; y: 200
        suggestionModel: ["One", "Two", "Three"]
    }
}
