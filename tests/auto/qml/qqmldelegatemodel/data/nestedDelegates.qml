import QtQuick
import QtQml.Models

Item {
    property QtObject modelProxy: QtObject {
        property list<Model> models: [Model {}]
    }

    DelegateModel {
        id: labelsModel
        model: modelProxy.models
        delegate: Loader {
            objectName: "loader"
            sourceComponent: TimeMarks {
                model: modelData
            }
        }
    }

    Repeater {
        model: labelsModel
    }
}
