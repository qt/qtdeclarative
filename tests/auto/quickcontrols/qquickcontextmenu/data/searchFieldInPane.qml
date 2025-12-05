import QtQuick
import QtQuick.Controls

Pane {
    width: 400
    height: 400

    property alias editor: searchField.contentItem

    SearchField {
        id: searchField
        width: parent.width
        suggestionModel: ListModel {
            ListElement { value: "123,456" }
        }
        textRole: "value"
    }
}
