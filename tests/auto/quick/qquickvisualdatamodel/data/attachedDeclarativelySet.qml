import QtQml
import QtQml.Models


QtObject {
    id: root
    property bool includeAll: false
    property alias count: instantiator.count
    readonly property DelegateModel mprop: DelegateModel {
        id: dm
        model: 10

        delegate: QtObject {
            required property int index
            DelegateModel.inItems: index % 2 === 0 || root.includeAll
        }
    }

    readonly property Instantiator instProp: Instantiator {
        id: instantiator
        model: dm
    }
}
