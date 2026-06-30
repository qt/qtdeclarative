// Identical to EmptyListRoleViaModels.qml, except it reaches ListModel/ListElement through
// "import QtQml" (which auto-imports QtQml.Models) instead of naming QtQml.Models directly.
import QtQml

QtObject {
    property ListModel model: ListModel {
        ListElement {
            attributes: []
        }
    }
    property int attributesCount: model.get(0).attributes.count
}
