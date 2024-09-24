import QtQuick
import Test

Window {
    id: root
    title: listView.count

    property alias listView: listView
    property ProxySourceModel connectionModel: null

    Component {
        id: modelComponent
        ProxySourceModel {}
    }

    ListView {
        id: listView
        anchors.fill: parent

        delegate: Text {
            text: model.Name
        }

        model: ProxyModel {
            sourceModel: root.connectionModel
        }
    }

    Component.onCompleted: root.connectionModel = modelComponent.createObject(root)
}
