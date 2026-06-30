// Imports QtQml.Models *by name*. qmlcachegen recognizes this and re-attaches the
// "attributes: []" source to the binding, so the empty-array list role survives
// ahead-of-time compilation even though this module is built with DISCARD_QML_CONTENTS.
import QtQml
import QtQml.Models

QtObject {
    property ListModel model: ListModel {
        ListElement {
            attributes: []
        }
    }
    property int attributesCount: model.get(0).attributes.count
}
